/*
	Internal functionality for the ECS.
*/

#ifndef ECS_MANAGER_H
#define ECS_MANAGER_H

#include "ecs.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ECS {
	hashtable_t *components;
	hashtable_t *systems;
	
	hashtable_t *entities;
	hash_t _last_entity;
	
	hashtable_t *cm_types;
};

typedef struct {
	void *ptr;
	size_t size;
	size_t count;
	size_t capacity;
} ComponentArray;

typedef struct {
	const char *type;
	component_create_func cr_func;
	component_delete_func dl_func;
	size_t type_size;
	hash_t type_hash;
	ComponentArray *components;
} ComponentType;

/* -------------------------------------------------------------------------- */

ComponentArray* Manager_CreateComponentArray(ECS *ecs, ComponentType *type, size_t cap);

ComponentType* Manager_GetComponentType(ECS *ecs, const char *type);
bool Manager_RegisterComponentType(ECS *ecs, ComponentType *type);

Component* Manager_CreateComponent(ECS *ecs, ComponentType *type);
void Manager_DeleteComponent(ECS *ecs, Component *comp);

Entity* Manager_CreateEntity(ECS *ecs);
void Manager_DeleteEntity(ECS *ecs, Entity *entity);

#endif
