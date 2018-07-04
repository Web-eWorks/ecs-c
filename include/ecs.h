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

typedef struct Event Event;

typedef void Component;
typedef struct ComponentInfo ComponentInfo;

typedef struct Entity Entity;

typedef void System;
typedef struct SystemInfo SystemInfo;

typedef struct ECS ECS;

/*
    Includes.
*/

#include "hash.h"
#include "dynarray.h"
#include "event.h"

#include "component.h"
#include "entity.h"
#include "system.h"

/*
	Create and destroy an ECS.
*/
ECS* ECS_New();
void ECS_Delete(ECS *ecs);

/*
    Trigger an ECS update.
*/
bool ECS_UpdateBegin(ECS *ecs);

/*
    Update a system.
*/
bool ECS_UpdateSystem(ECS *ecs, const char *name);

/*
    End an ECS update.
*/
void ECS_UpdateEnd(ECS *ecs);

#endif
