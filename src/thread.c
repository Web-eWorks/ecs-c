// A multithreaded update implementation.

#include "manager.h"
#include "profile.h"

#define THREAD_LOCK(data) pthread_mutex_lock(&data->update_mutex)
#define THREAD_UNLOCK(data) pthread_mutex_unlock(&data->update_mutex)

bool UpdateThread_start(ThreadData *data)
{
    assert(data && data->ecs);

    data->collection_size = 32;
    data->collection = calloc(data->collection_size, sizeof(ComponentInfo *));

    if (!data->collection) {
        return false;
    }

    return true;
}

void UpdateThread_update(ThreadData *data, System *system, Entity *entity)
{
    size_t collection_size = system->collection.size;
    // Resize the buffer if it needs it.
    if (data->collection_size < collection_size) {
        ComponentInfo **ptr = calloc(collection_size, sizeof(ComponentInfo *));
        if (!ptr) {
            ECS_Error(data->ecs, "Failed to resize thread component buffer!");
            data->running = false;
            return;
        }

        data->collection_size = collection_size;
        data->collection = ptr;
    }

    // Because we're not inserting or deleting components from the ECS or entity
    // during threaded update steps, getting components is thread safe.
	for (size_t idx = 0; idx < collection_size; idx++) {
		ComponentInfo *comp = ECS_EntityGetComponentOfType(
			entity, system->collection.types[idx], 0);
		data->collection[idx] = comp;
	}

	system->up_func(entity, data->collection, system->udata);
}

void UpdateThread_end(ThreadData *data)
{
    data->running = false;
    free(data->collection);
    pthread_exit(NULL);
}

void* UpdateThread_main(void *arg)
{
    ThreadData *data = arg;
    data->running = UpdateThread_start(data);
    printf("Thread spawned!\n");

    // Wait until there's a new system chunk to update.
    while (data->running) {
        pthread_mutex_lock(&data->update_mutex);
        data->ready = true;
        pthread_cond_signal(&data->ecs->ready_cond);
        while (data->ready) pthread_cond_wait(&data->update_cond, &data->update_mutex);

        // While data->ready == false, the ECS will not write to data.
        pthread_mutex_unlock(&data->update_mutex);

        if (data->system->collection.size == 0) {
            UpdateThread_update(data, data->system, NULL);
            continue;
        }

        // Update the assigned chunk of the system.
        hashtable_t *ht = data->system->ent_queue;
        HT_RANGE_FOR(ht, data->range.start, data->range.end) {
            UpdateThread_update(data, data->system, ht_get(data->ecs->entities, idx));
        }
    }

    UpdateThread_end(data);
    return NULL;
}

void ThreadData_delete(ThreadData *data)
{
    pthread_mutex_destroy(&data->update_mutex);
    pthread_cond_destroy(&data->update_cond);
    free(data->collection);
}
