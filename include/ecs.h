// ecs.h

/*
	Most interaction with the 'global' portion of the ECS will be taking place
	using these functions.
*/

#ifndef ECS_MAIN_H
#define ECS_MAIN_H

#include <stdbool.h>

/*
	Forward declarations.
*/

typedef void Component;
typedef struct ComponentInfo ComponentInfo;

typedef struct Entity Entity;

typedef void System;
typedef struct SystemInfo SystemInfo;

typedef struct ECS ECS;

#include "hash.h"
#include "component.h"
#include "entity.h"
#include "system.h"

/*
	Create and destroy an ECS
*/
ECS* ECS_New();
void ECS_Delete(ECS *ecs);

/*
	Add an entity to the ECS.
*/
int ECS_AddEntity(ECS *ecs, Entity *entity);

/*
	Remove an entity from the ECS.
*/
void ECS_RemoveEntity(ECS *ecs, Entity *entity);

#endif
