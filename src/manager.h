/*
	Internal functionality for the ECS.
*/

#ifndef ECS_MANAGER_H
#define ECS_MANAGER_H

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "ecs.h"

struct ECS {
	hashtable_t *components;
	hashtable_t *systems;
	hashtable_t *entities;

	// incremental counters for generating ids
	hash_t _last_entity;
	hash_t _last_component;
	hash_t _last_system;

	// type registries
	hashtable_t *cm_types;
};

struct SystemInfo {
    const char *name;
    hash_t name_hash;
    system_update_func up_func;
    system_collection_func coll_func;
    system_event_func ev_func;
    EventQueue *ev_queue;
    void *udata;
};

typedef struct {
	const char *type;
	component_create_func cr_func;
	component_delete_func dl_func;
	size_t type_size;
	hash_t type_hash;
	dynarray_t components;
} ComponentType;

/* -------------------------------------------------------------------------- */

ComponentType* Manager_GetComponentType(ECS *ecs, const char *type);
bool Manager_RegisterComponentType(ECS *ecs, ComponentType *type);

ComponentInfo* Manager_CreateComponent(ECS *ecs, ComponentType *type);
ComponentInfo* Manager_GetComponent(ECS *ecs, hash_t id);
void Manager_DeleteComponent(ECS *ecs, ComponentInfo *comp);

Entity* Manager_CreateEntity(ECS *ecs);
Entity* Manager_GetEntity(ECS *ecs, hash_t id);
void Manager_DeleteEntity(ECS *ecs, Entity *entity);

bool Manager_RegisterSystem(ECS *ecs, SystemInfo *info);
SystemInfo* Manager_GetSystem(ECS *ecs, const char *name);
void Manager_UnregisterSystem(ECS *ecs, const char *name);

#endif
