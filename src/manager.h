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
void Manager_DeleteComponent(ECS *ecs, ComponentInfo *comp);

Entity* Manager_CreateEntity(ECS *ecs);
void Manager_DeleteEntity(ECS *ecs, Entity *entity);

#endif
