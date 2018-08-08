#include "ecs.h"
#include "manager.h"
#include "profile.h"

ECS* ECS_New()
{
	// Default allocation granularity.
	// Each application should perform their own testing, but these are the best
	// values found for the test harness application.
	const ECS_AllocInfo alloc = {
		// Components and entities
		256, 256,
		// Systems and component types
		32, 32,
		// Number of components / entity
		8,
		// Number of entities systems will operate on.
		256
	};
	return ECS_CustomNew(&alloc);
}

ECS* ECS_CustomNew(const ECS_AllocInfo *alloc)
{
	assert(alloc);

	ECS *ecs = malloc(sizeof(ECS));
	if (!ecs) return NULL;

	// Properly free all memory if we can't create the ECS.
	bool ok = true;

	ecs->entities = ha_alloc(alloc->entities, sizeof(Entity));
	ok = ok && ecs->entities;

	ecs->systems = ht_alloc(alloc->systems, sizeof(System));
	ok = ok && ecs->systems;
	ok = ok && dyn_alloc(&ecs->update_systems, alloc->systems, sizeof(System *));

	ecs->cm_types = ht_alloc(alloc->cm_types, sizeof(ComponentType));
	ecs->alloc_info = *alloc;
	ok = ok && ecs->cm_types;

	ecs->num_threads = 0;
	ecs->ready_threads = 0;
	ecs->threads = NULL;
	ecs->buffers = ha_alloc(4, sizeof(CommandBuffer));
	ok = ok && ecs->buffers;

	ok = ok && pthread_mutex_init(&ecs->global_lock, NULL) == 0;
	ok = ok && pthread_cond_init(&ecs->ready_cond, NULL) == 0;

	ecs->is_updating = false;

	if (!ok) {
		ECS_Delete(ecs);
		return NULL;
	}

	return ecs;
}

bool ECS_SetThreads(ECS *ecs, size_t threads)
{
	assert(ecs && !ecs->is_updating);

	// TODO: create and spawn threads.
	int nthreads = threads - ecs->num_threads;
	if (nthreads <= 0) {
		ECS_UNLOCK(ecs);
		return true;
	}

	ThreadData **ptr = realloc(ecs->threads, sizeof(ThreadData *) * threads);
	if (!ptr) return false;
	for (size_t idx = ecs->num_threads; idx < threads; idx++) {
		ThreadData *data = ptr[idx] = malloc(sizeof(ThreadData));
		if (!data) {
			ECS_Error(ecs, "Could not allocate memory for new threads!");
			return false;
		}

		data->ecs = ecs;
		data->running = false;
		data->ready = false;
		pthread_mutex_init(&data->update_mutex, NULL);
		pthread_cond_init(&data->update_cond, NULL);
		pthread_create(&data->thread, NULL, &UpdateThread_main, data);
	}

	ecs->num_threads = threads;
	ecs->threads = ptr;

	return true;
}

void ECS_Delete(ECS *ecs)
{
	assert(ecs);

	if (ecs->num_threads > 0 && ecs->threads) {
		for (size_t idx = 0; idx < ecs->num_threads; idx++) {
			pthread_cancel(ecs->threads[idx]->thread);
			pthread_join(ecs->threads[idx]->thread, NULL);
			ThreadData_delete(ecs->threads[idx]);
			free(ecs->threads[idx]);
		}
		free(ecs->threads);
	}

	// Deleting entities will delete all attached components, which make up
	// the extreme majority of all components.
	if (ecs->entities) {
		Entity *entity;
		HA_FOR(ecs->entities, entity, 0) ECS_EntityDelete(entity);
		ha_free(ecs->entities);
	}

	if (ecs->systems) {
		HT_FOR(ecs->systems, 0) {
			System *system = ht_get(ecs->systems, idx);
			if (system) Manager_UnregisterSystem(ecs, system);
		}
		ht_free(ecs->systems);
	}
	dyn_free(&ecs->update_systems);

	// There are only a handful of component deletions to perform at this point.
	if (ecs->cm_types) {
		HT_FOR(ecs->cm_types, 0) {
			ComponentType *type = ht_get(ecs->cm_types, idx);
			if (type) {
				ht_free(type->components);
				free((char *)type->type);
				ht_delete(ecs->cm_types, idx);
			}
		}
		ht_free(ecs->cm_types);
	}

	if (ecs->buffers) {
		CommandBuffer *buff;
		HA_FOR(ecs->buffers, buff, 0) {
			CommandBuffer_Delete(buff);
		}
		ha_free(ecs->buffers);
	}

	free(ecs);
}

void ECS_Error(ECS *ecs, const char *error)
{
	ECS_LOCK(ecs);
	fprintf(stderr, "%s\n", error);
	ECS_UNLOCK(ecs);
}

/*
	TODO: Allow systems to define their order of execution, and handle circular
	referencing in the systems.
*/
void ECS_ArrangeSystems(ECS *ecs)
{

}

// Create entity queues for each system, resolve the order of systems, etc.
bool ECS_UpdateBegin(ECS *ecs)
{
	// Iterate systems.
	HT_FOR(ecs->systems, 0) {
		System *system = ht_get(ecs->systems, idx);
		// queue the system for updates
		dyn_insert(&ecs->update_systems, ecs->update_systems.size, &system);
	}

	ECS_ArrangeSystems(ecs);

	return true;
}

#define THREAD_MIN_LOAD 100

// wait for all threads to finish
static void synchronize_threads(ECS *ecs)
{
	ECS_LOCK(ecs);
	while (true) {
		if (ecs->ready_threads == ecs->num_threads) {
			ECS_UNLOCK(ecs);
			return;
		}
		pthread_cond_wait(&ecs->ready_cond, &ecs->global_lock);
	}
}

static void distribute_to_thread(ECS *ecs, ThreadData *thread, System *system, hash_t start, hash_t end)
{
	pthread_mutex_lock(&thread->update_mutex);

	// Pass along the data.
	thread->system = system;
	thread->range.start = start;
	thread->range.end = end;
	thread->ready = false;

	UNREADY_THREAD(ecs);

	// Let it get to work.
	pthread_cond_signal(&thread->update_cond);
	pthread_mutex_unlock(&thread->update_mutex);
}

// Get the number of ready threads.
static size_t ready_threads(ECS *ecs)
{
	ECS_LOCK(ecs);
	size_t ret = ecs->ready_threads;
	ECS_UNLOCK(ecs);
	return ret;
}

static void wait_until_ready(ECS *ecs, size_t num_threads)
{
	if (ecs->num_threads < num_threads) return;

	ECS_LOCK(ecs);
	while(ecs->ready_threads < num_threads) {
		pthread_cond_wait(&ecs->ready_cond, &ecs->global_lock);
	}
	ECS_UNLOCK(ecs);
}

// Get the first ready thread index.
static bool ready_thread(ECS *ecs, size_t *thread_idx)
{
	if (!thread_idx) return false;

	for (size_t idx = *thread_idx; idx < ecs->num_threads; idx++) {
		ThreadData *data = ecs->threads[idx];
		THREAD_LOCK(data);
		bool ready = data->ready;
		THREAD_UNLOCK(data);
		if (ready) {
			*thread_idx = idx;
			return true;
		}
	}

	return false;
}

// TODO: we assume all threads are ready here - rewrite this to support unready
// threads from multiple-system queueing.
void ECS_DistributeSystemUpdate(ECS *ecs, System *system)
{
	size_t ents = ha_len(system->ent_queue) / ecs->num_threads;

	// If we don't have enough to justify queuing to more than one thread, (or
	// only have one thread) just dispatch the queue.
	if (system->collection.size == 0 || ecs->num_threads == 1 || ents < THREAD_MIN_LOAD) {
		distribute_to_thread(ecs, ecs->threads[0], system, 0, 0);
		return;
	}

	hash_t last = 0;
	// TODO: verify that all entities in the queue are actually getting updated,
	// and that we're not losing one at the beginning or end.
	for (size_t idx = 0; idx < ecs->num_threads; idx++) {
		hash_t curr = last + ents;
		distribute_to_thread(ecs, ecs->threads[idx], system, last, curr);
		last = curr;
	}
}

// Update a single system.
void ECS_UpdateSystem(ECS *ecs, System *system)
{
	if (ecs->num_threads > 0 && system->is_thread_safe) {
		ECS_DistributeSystemUpdate(ecs, system);
	}
	// Update the system with all entities that have the correct components.
	else if (system->collection.size > 0) {
		hash_t *hash;
		HA_FOR(system->ent_queue, hash, 0) {
			Entity *entity = ha_get(ecs->entities, *hash);
			if (!entity) continue;

			Manager_UpdateSystem(ecs, system, entity);
		}
	}
	// If the system doesn't operate on components, we only update it once.
	else {
		Manager_UpdateSystem(ecs, system, NULL);
	}

	if (system->ev_func) {
		Event *ev = NULL;
		while ((ev = EventQueue_Peek(system->ev_queue, 0)) != NULL) {
			Manager_SystemEvent(ecs, system, ev);
			EventQueue_Pop(system->ev_queue, NULL);
		}
	}
}

void ECS_ResolveCommandBuffers(ECS *ecs)
{
	/* TODO: refactor these for the new APIs
	CommandBuffer *buff;
	HA_FOR(ecs->buffers, buff, 0) {
		hashtable_t *ht = ht_alloc(32, sizeof(hash_t));
		for (size_t idx = 0; idx < buff->commands.size; idx++) {
			Command *cm = dyn_get(&buff->commands, idx);

			hash_t id = cm->data[0];
			if (ht_get(ht, id)) id = *(hash_t *)ht_get(ht, id);
			Entity *e = ECS_EntityGet(ecs, id);

			if (cm->type == CMD_EntityCreate) {
				e = ECS_EntityNew(ecs);
				ht_insert(ht, cm->data[0], &e->id);
			}
			else if (cm->type == CMD_EntityDelete) {
				if (e) ECS_EntityDelete(e);
			}
			else if (cm->type == CMD_ComponentAttach) {
				ComponentInfo *c = ECS_ComponentGet(ecs, cm->data[1]);
				if (e && c) ECS_EntityAddComponent(e, c);
			}
			else if (cm->type == CMD_ComponentDetach) {
				if (e) ECS_EntityRemoveComponent(e, cm->data[1]);
			}
		}

		ht_free(ht);
		CommandBuffer_Delete(buff);
	}
	*/
}

/*
	Each system (nominally) updates on one entity at a time, and then only on
	certain components on those entities.

	Each system, when registered, should provide data regarding what components
	they read from or write to, as well as if they access other entities. Systems
	can also specify if they want to run before or after other systems by name.

	The ECS will then resolve the order of execution of systems. The ECS may
	schedule two systems simultaneously if they are not dependant upon each other
	and do not involve the same components.

	Each system's update function will be called for each valid entity the system
	operates on. The system update is not guaranteed to take place on the main
	(or same) thread, nor is it guaranteed to happen in isolation - multiple
	entities may be updated at the same time on different threads.

	To preserve performance and avoid race conditions or other threading bugs,
	when updating entities, entity creation and deletion must be queued in a
	command buffer until a synchronization point is reached in the update cycle.
*/
bool ECS_UpdateSystems(ECS *ecs)
{
	HT_FOR(ecs->systems, 0) {
		System *system = ht_get(ecs->systems, idx);
		ECS_UpdateSystem(ecs, system);

		// TODO: allow queuing multiple systems when necessary.
		if (ecs->num_threads > 0) {
			synchronize_threads(ecs);
		}
		if (ha_len(ecs->buffers) > 0) {
			ECS_ResolveCommandBuffers(ecs);
		}
	}

	return true;
}

void ECS_UpdateEnd(ECS *ecs)
{
	(void) ecs;
}
