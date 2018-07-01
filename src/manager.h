/*
	Internal functionality for the ECS.
*/

#ifndef ECS_MANAGER_H
#define ECS_MANAGER_H

#include "ecs.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ECS {
	hashtable_t components;
	hashtable_t systems;
	hashtable_t entities;
};

typedef struct {
	const char *type;
	component_create_func cr_func;
	component_delete_func dl_func;
	size_t type_size;
	hash_t type_hash;
} ComponentType;

ComponentType* Manager_GetComponentType(ECS *ecs, const char *type);
int Manager_RegisterComponentType(ECS *ecs, ComponentType *type);

Component* Manager_CreateComponent(ECS *ecs, ComponentType *type);
void Manager_DeleteComponent(ECS *ecs, Component *comp);

Entity* Manager_CreateEntity(ECS *ecs);
void Manager_DeleteEntity(ECS *ecs, Entity *entity);

#endif
