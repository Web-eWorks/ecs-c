// ecs.h

/*
	Most interaction with the 'global' portion of the ECS will be taking place
	using these functions.
*/

#ifndef ECS_MAIN_H
#define ECS_MAIN_H

#include <stdbool.h>
#include <stddef.h>

/*
	Forward declarations.
*/

typedef struct Event Event;

typedef void Component;
typedef struct ComponentInfo ComponentInfo;

typedef struct Entity Entity;

/*
    An encapsulation of update code.
*/
typedef struct System System;

/*
    The core datastructure of the ECS.
*/
typedef struct ECS ECS;

/*
    The performance of the ECS is affected by the contiguous nature of it's
    data storage.

    The default values provide a fairly good tradeoff between memory
    fragmentation and consumption, but an application may pass its own values
    for these allocations.
*/
typedef struct {
    size_t components;
    size_t entities;
    size_t systems;
    size_t cm_types;

    size_t entity_components;
    size_t system_entities;
} ECS_AllocInfo;

/*
    Includes.
*/

#include "hash.h"
#include "hasharray.h"
#include "dynarray.h"
#include "event.h"

#include "component.h"
#include "entity.h"
#include "system.h"

/*
	Create and destroy an ECS.
*/
ECS* ECS_New();
ECS* ECS_CustomNew(const ECS_AllocInfo *alloc);
void ECS_Delete(ECS *ecs);

/*
    Sets the number of threads the ECS will use for system updates.

    This function will only increase the number of threads; subsequent calls
    with a lower number will not delete spawned threads.
*/
bool ECS_SetThreads(ECS *ecs, size_t threads);

/*
    Trigger an ECS update.
*/
bool ECS_UpdateBegin(ECS *ecs);

/*
    Update all systems.
*/
bool ECS_UpdateSystems(ECS *ecs);

/*
    End an ECS update.
*/
void ECS_UpdateEnd(ECS *ecs);

#endif
