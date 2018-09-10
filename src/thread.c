// A multithreaded update implementation.

#include "manager.h"
#include "profile.h"

#define THREAD_WAIT(data) pthread_cond_wait(&data->update_cond, &data->update_mutex)

bool UpdateThread_start(ThreadData *data)
{
    assert(data && data->ecs);

    data->collection_size = 32;
    data->collection = calloc(data->collection_size, sizeof(Component *));

    if (!data->collection) {
        return false;
    }

    return true;
}

void UpdateThread_update(ThreadData *data, System *system, Entity entity)
{
    size_t collection_size = system->archetype->size;
    // Resize the buffer if it needs it.
    if (data->collection_size < collection_size) {
        Component **ptr = calloc(collection_size, sizeof(Component *));
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
        ComponentID id = {entity, system->archetype->components[idx]};
        data->collection[idx] = Manager_GetComponentByID(data->ecs, id);
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
    hasharray_t *entities = data->ecs->entities;

    // Wait until there's a new system chunk to update.
    while (data->running) {
        THREAD_LOCK(data);
        data->ready = true;
        READY_THREAD(data->ecs);
        pthread_cond_signal(&data->ecs->ready_cond);
        while (data->ready) THREAD_WAIT(data);

        System *system = data->system;
        hasharray_t *ha = system->ent_queue;
        size_t start = data->range.start;
        size_t end = data->range.end;
        THREAD_UNLOCK(data);

        if (system->archetype->size == 0) {
            UpdateThread_update(data, system, 0);
            continue;
        }

        hash_t *hash;
        // Update the assigned chunk of the system.
        if (end == 0) HA_FOR(ha, hash, start) {
            UpdateThread_update(data, system, idx);
        }
        else HA_RANGE_FOR(ha, hash, start, end) {
            UpdateThread_update(data, system, idx);
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
