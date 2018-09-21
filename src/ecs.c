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
		// Number of entities systems will operate on.
		256
	};
	return ECS_CustomNew(&alloc);
}

ECS* ECS_CustomNew(const ECS_AllocInfo *alloc)
{
	// Properly free all memory if we can't create the ECS.
	#define _ERR(cond) ERR(cond, ECS_Delete(ecs); return NULL, "Error creating ECS.\n")

	assert(alloc);

	ECS *ecs = malloc(sizeof(ECS));
	if (!ecs) return NULL;

	_ERR(ecs->entities = ha_alloc(alloc->entities, sizeof(Entity)));

	_ERR(ecs->systems = ht_alloc(alloc->systems, sizeof(System)));
	_ERR(dyn_alloc(&ecs->system_order, alloc->systems, sizeof(System *)));
	_ERR(dyn_alloc(&ecs->update_systems, alloc->systems, sizeof(SystemQueueItem)));

	_ERR(ecs->cm_types = ht_alloc(alloc->cm_types, sizeof(ComponentType)));

	ecs->alloc_info = *alloc;
	ecs->num_threads = 0;
	ecs->ready_threads = 0;
	ecs->threads = NULL;
	_ERR(ecs->buffers = ha_alloc(4, sizeof(CommandBuffer)));

	_ERR(pthread_mutex_init(&ecs->global_lock, NULL) == 0);
	_ERR(pthread_cond_init(&ecs->ready_cond, NULL) == 0);

	ecs->is_updating = false;

	#undef _ERR

	return ecs;
}

bool ECS_SetThreads(ECS *ecs, size_t threads)
{
	assert(ecs && !ecs->is_updating);

	int nthreads = threads - ecs->num_threads;
	if (nthreads <= 0) {
		ECS_UNLOCK(ecs);
		return true;
	}

	ThreadData **ptr = realloc(ecs->threads, sizeof(ThreadData *) * threads);
	if (!ptr) return false;
	for (size_t idx = ecs->num_threads; idx < threads; idx++) {
		ptr[idx] = ECS_NewThread(ecs);
		ERR_RET_ZERO(ptr[idx], "Error creating new thread %ld.\n", idx);
	}

	ecs->num_threads = threads;
	ecs->threads = ptr;

	// We've changed the number of threads, so we need to rearrange the queue to
	// take advantage of that.
	ecs->update_systems_dirty = true;

	return true;
}

void ECS_Delete(ECS *ecs)
{
	assert(ecs);

	// Clean up threads.
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
		HA_FOR(ecs->entities, entity, 0) ECS_EntityDelete(ecs, idx);
		ha_free(ecs->entities);
	}

	if (ecs->systems) {
		HT_FOR(ecs->systems) {
			System *system = ht_get(ecs->systems, idx);
			if (system) Manager_UnregisterSystem(ecs, system);
		}
		ht_free(ecs->systems);
	}
	dyn_free(&ecs->update_systems);
	dyn_free(&ecs->system_order);

	// There are only a handful of component deletions to perform at this point.
	if (ecs->cm_types) {
		HT_FOR(ecs->cm_types) {
			ComponentType *type = ht_get(ecs->cm_types, idx);
			if (type) {
				ht_free(type->components);
				free((char *)type->type);
				ht_delete(ecs->cm_types, idx);
			}
		}
		ht_free(ecs->cm_types);
	}

	// Destroy the command buffers
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

/* -------------------------------------------------------------------------- */

#define THREAD_MIN_LOAD 1000

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

static void distribute_to_thread(ECS *ecs, ThreadData *thread, SystemQueueItem *item)
{
	pthread_mutex_lock(&thread->update_mutex);

	// Pass along the data.
	thread->system = item->system;
	thread->range.start = item->start;
	thread->range.end = item->end;
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

// TODO: this is O(n^2) with regards to the length of the archetypes involved
// Fix this yesterday.
static bool systems_in_parallel(System *a, System *b)
{
	// Essentially, just see if there's any overlap between the two.
	for (size_t idxa = 0; idxa < a->archetype->size; idxa++) {
		for (size_t idxb = 0; idxb < b->archetype->size; idxb++) {
			if (a->archetype->components[idxa] == b->archetype->components[idxb])
				return false;
		}
	}

	return true;
}

static bool system_requires_barrier(ECS *ecs, System *system)
{
	// Walk back through the queue until we find the start, a barrier, or a
	// system that would cause a conflict.
	if (!system->is_thread_safe) return true;

	dynarray_t *arr = &ecs->update_systems;
	for (size_t idx = arr->size - 1; idx > 0; --idx) {
		SystemQueueItem *item = dyn_get(arr, idx);
		if (item->type == SYSTEM_UPDATE_BARRIER) return false;
		if (!systems_in_parallel(system, item->system)) return true;
	}

	return false;
}

/*
	TODO: arrange systems in the most optimal way.
*/
void ECS_ArrangeSystems(ECS *ecs)
{
	bool is_thread_safe = true;

	#define INSERT(t) dyn_append(&ecs->update_systems, &t);

	DYN_FOR(ecs->system_order, 0) {
		System *system = *(System **)dyn_get(&ecs->system_order, idx);

		SystemQueueItem item = {
			SYSTEM_UPDATE_QUEUED,
			ha_get(system->ent_queue, 0) ? 0 : ha_next(system->ent_queue, 0),
			ha_last(system->ent_queue),
			system
		};

		// Insert a barrier if this system conflicts
		if (!is_thread_safe && system_requires_barrier(ecs, system)) {
			SystemQueueItem barrier = {SYSTEM_UPDATE_BARRIER, 0, 0, NULL};
			INSERT(barrier);
			is_thread_safe = true;
		}

		// If we're onthread, we've got a barrier up and can safely queue here.
		if (!system->is_thread_safe) {
			item.type = SYSTEM_UPDATE_ONTHREAD;
			INSERT(item);
			continue;
		}

		// Otherwise, we've got threads running in the background.
		is_thread_safe = false;

		// If we have enough items, split them across multiple threads.
		if (ecs->num_threads > 1 && ha_len(system->ent_queue) > THREAD_MIN_LOAD) {
			// Ensure that we're splitting things up relatively evenly.
			size_t num_threads = round((float)item.end / THREAD_MIN_LOAD);
			if (num_threads > ecs->num_threads) num_threads = ecs->num_threads;

			// Insert jobs for each thread.
			size_t ents = item.end / num_threads;
			for (size_t idx = 0; idx < num_threads; idx++) {
				item.start = idx * ents;
				if(!ha_get(system->ent_queue, item.start))
					item.start = ha_next(system->ent_queue, item.start);
				item.end = (idx + 1) * ents;
				INSERT(item);
			}
		// Otherwise, just use one thread.
		} else {
			INSERT(item);
		}
	}

	#undef INSERT
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

// Distribute an update to a thread.
void ECS_DispatchSystemUpdate(ECS *ecs, SystemQueueItem *item)
{
	if (item->type == SYSTEM_UPDATE_QUEUED && ecs->num_threads > 0) {
		wait_until_ready(ecs, 1);
		size_t thread_idx = 0;
		// At least one thread is ready. Assert if this is false.
		assert(ready_thread(ecs, &thread_idx));
		distribute_to_thread(ecs, ecs->threads[thread_idx], item);
	}
	else {
		Entity *entity;
		if (item->start == item->end && item->end == 0) {
			Manager_UpdateSystem(ecs, item->system, 0);
			return;
		}

		HA_RANGE_FOR(item->system->ent_queue, entity, item->start, item->end) {
			Manager_UpdateSystem(ecs, item->system, *entity);
		}
	}
}

// Resolve a barrier
void ECS_DispatchBarrier(ECS *ecs, SystemQueueItem *item)
{
	synchronize_threads(ecs);

	if (ha_len(ecs->buffers) > 0) {
		ECS_ResolveCommandBuffers(ecs);
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
void ECS_Update(ECS *ecs)
{
	// If it needs it, update the queue.
	if (ecs->update_systems_dirty) {
		ECS_ArrangeSystems(ecs);
		ecs->update_systems_dirty = false;
	}

	// Dispatch all updates and resolve barriers.
	for (size_t idx = 0; idx < ecs->update_systems.size; idx++) {
		SystemQueueItem *item = dyn_get(&ecs->update_systems, idx);

		switch (item->type) {
		case SYSTEM_UPDATE_BARRIER:
			ECS_DispatchBarrier(ecs, item);
			break;
		case SYSTEM_UPDATE_ONTHREAD:
			synchronize_threads(ecs);
		case SYSTEM_UPDATE_QUEUED:
			ECS_DispatchSystemUpdate(ecs, item);
			break;
		default:
			break;
		}
	}

	// Dispatch events.
	HT_FOR(ecs->systems) {
		System *system = ht_get(ecs->systems, idx);
		DYN_FOR(*system->ev_queue, 0) {
			Event *ev = EventQueue_Peek(system->ev_queue, idx);
			Manager_SystemEvent(ecs, system, ev);
		}
		EventQueue_Clear(system->ev_queue);
	}

	synchronize_threads(ecs);
}
