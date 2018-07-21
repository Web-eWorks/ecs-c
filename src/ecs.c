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
		16,
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

	ecs->components = ha_alloc(alloc->components, sizeof(ComponentInfo));
	ecs->_last_component = 1;
	ok = ok && ecs->components;

	ecs->entities = ha_alloc(alloc->entities, sizeof(Entity));
	ecs->_last_entity = 1;
	ok = ok && ecs->entities;

	ecs->systems = ht_alloc(alloc->systems, sizeof(System));
	ok = ok && ecs->systems;
	ecs->_last_system = 1;
	ok = ok && dyn_alloc(&ecs->update_systems, alloc->systems, sizeof(System *));

	ecs->cm_types = ht_alloc(alloc->cm_types, sizeof(ComponentType));
	ecs->alloc_info = *alloc;
	ok = ok && ecs->cm_types;

	ecs->num_threads = 0;
	ecs->ready_threads = 0;
	ecs->threads = NULL;

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

	// There are only a handful of component deletions to perform at this point.
	if (ecs->components) {
		ComponentInfo *comp;
		HA_FOR(ecs->components, comp, 0) ECS_ComponentDelete(ecs, comp);
		ha_free(ecs->components);
	}

	if (ecs->systems) {
		HT_FOR(ecs->systems, 0) {
			System *system = ht_get(ecs->systems, idx);
			if (system) Manager_UnregisterSystem(ecs, system);
		}
		ht_free(ecs->systems);
	}
	dyn_free(&ecs->update_systems);

	if (ecs->cm_types) {
		HT_FOR(ecs->cm_types, 0) {
			ComponentType *type = ht_get(ecs->cm_types, idx);
			if (type) {
				mp_destroy(type->components);
				free((char *)type->type);
				ht_delete(ecs->cm_types, idx);
			}
		}
		ht_free(ecs->cm_types);
	}

	free(ecs);
}

void ECS_Error(ECS *ecs, const char *error)
{
	ECS_LOCK(ecs);
	fprintf(stderr, "%s\n", error);
	ECS_UNLOCK(ecs);
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

	// TODO: allow systems to define their order of execution.
	// TODO: handle circular referencing in the systems.
	// TODO: maybe make system ordering an issue of the caller?

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
	for (size_t idx = 0; idx < ecs->num_threads; idx++) {
		hash_t curr = last + ents;
		distribute_to_thread(ecs, ecs->threads[idx], system, last, curr);
		last = curr;
	}
}

// Update a single system.
static void ECS_UpdateSystem(ECS *ecs, System *system)
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
		if (ecs->num_threads > 0) {
			synchronize_threads(ecs);
		}
	}

	return true;
}

void ECS_UpdateEnd(ECS *ecs)
{
	(void) ecs;
}
