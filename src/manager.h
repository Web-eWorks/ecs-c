/*
	Internal functionality for the ECS.
*/

#ifndef ECS_MANAGER_H
#define ECS_MANAGER_H

#include <assert.h>
#include <math.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "ecs.h"
#include "command_buffer.h"

typedef struct SystemCollection SystemCollection;
typedef struct ComponentType ComponentType;
typedef struct ThreadData ThreadData;

struct ECS {
	hasharray_t *entities;
	hashtable_t *systems;
	// component type registry
	hashtable_t *cm_types;

	dynarray_t update_systems;
	ECS_AllocInfo alloc_info;

	bool is_updating;
	hasharray_t *buffers;

	size_t num_threads;
	size_t ready_threads;
	ThreadData **threads;

	pthread_mutex_t global_lock;
	pthread_cond_t ready_cond;
};

struct ThreadData {
    ECS *ecs;
	pthread_t thread;
	bool running;

	bool ready;
    pthread_mutex_t update_mutex;
	pthread_cond_t update_cond;

	size_t collection_size;
    Component **collection;

	System *system;
	// The range of entities to update.
	struct {
		hash_t start;
		hash_t end;
	} range;
};

void ThreadData_delete(ThreadData *data);

struct SystemCollection {
	size_t size;
	hash_t *types;
	Component **comps;
};

struct System {
    const char *name;
    hash_t name_hash;
	void *udata;

    system_update_func up_func;
    system_event_func ev_func;

	bool is_thread_safe;

	SystemCollection collection;
	hash_t *dependencies;
	size_t deps_size;

	EventQueue *ev_queue;
	hasharray_t *ent_queue;
};

struct ComponentType {
	const char *type;
	component_create_func cr_func;
	component_delete_func dl_func;
	size_t type_size;
	hash_t type_hash;
	hashtable_t *components;
};

/* -------------------------------------------------------------------------- */

void ECS_Error(ECS *ecs, const char *msg);
void ECS_DispatchSystemUpdate(ECS *ecs, System *system, Entity *entity);

void* UpdateThread_main(void *arg);

bool System_CreateCollection(SystemCollection *coll, const char **collection);
void System_DeleteCollection(SystemCollection *coll);

/* -------------------------------------------------------------------------- */

bool Manager_HasComponentType(ECS *ecs, hash_t type);
ComponentType* Manager_GetComponentType(ECS *ecs, hash_t type);
bool Manager_RegisterComponentType(ECS *ecs, ComponentType *type);

Component* Manager_CreateComponent(ECS *ecs, ComponentType *type, hash_t id);
Component* Manager_GetComponent(ECS *ecs, ComponentType *type, hash_t id);
// If a component exists under an entity's ID, it is automatically associated
// with that entity. No backsies.
Component* Manager_GetComponentByID(ECS *ecs, ComponentID id);
void Manager_DeleteComponent(ECS *ecs, ComponentType *type, hash_t id);

Entity* Manager_CreateEntity(ECS *ecs);
Entity* Manager_GetEntity(ECS *ecs, hash_t id);
void Manager_DeleteEntity(ECS *ecs, Entity *entity);

bool Manager_RegisterSystem(ECS *ecs, System *info);
System* Manager_GetSystem(ECS *ecs, const char *name);
void Manager_UnregisterSystem(ECS *ecs, System *system);

void Manager_UpdateCollections(ECS *ecs, Entity *entity);
bool Manager_ShouldSystemQueueEntity(ECS *ecs, System *sys, Entity *entity);
void Manager_UpdateSystem(ECS *ecs, System *info, Entity *entity);
void Manager_SystemEvent(ECS *ecs, System *info, Event *event);

/* -------------------------------------------------------------------------- */

#define ECS_ERROR(ecs, str, ...) { \
	char _error_msg[128]; \
	snprintf(_error_msg, 128, str, __VA_ARGS__); \
	ECS_Error(ecs, _error_msg); \
}

#define ECS_LOCK(ecs) pthread_mutex_lock(&ecs->global_lock)
#define ECS_UNLOCK(ecs) pthread_mutex_unlock(&ecs->global_lock)
#define ECS_ATOMIC(ecs, op) ECS_LOCK(ecs); op; ECS_UNLOCK(ecs)

#define THREAD_LOCK(data) pthread_mutex_lock(&data->update_mutex)
#define THREAD_UNLOCK(data) pthread_mutex_unlock(&data->update_mutex)

#define READY_THREAD(ecs) ECS_ATOMIC(ecs, (ecs)->ready_threads++)
#define UNREADY_THREAD(ecs) ECS_ATOMIC(ecs, (ecs)->ready_threads--)

#endif // ECS_MANAGER_H
