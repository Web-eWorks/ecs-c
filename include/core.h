// core.h

#ifndef ECS_CORE_H
#define ECS_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#include "hash.h"
#include "hasharray.h"
#include "dynarray.h"

/*
	Forward declarations.
*/

typedef struct Event Event;

/*
    A component is an encapsulation of a single set of data, usually related to
    a single purpose. Components are implemented as black-box data initialized
    and managed by external code, but allocated and stored by the ECS.

    Components are stored in arrays of type, indexed by their owning entity's ID.
*/
typedef void Component;

/*
    A simple way to refer to components.
*/
typedef struct ComponentID ComponentID;

/*
    The basic unit of objects. Each entity may contain one or more components.
*/
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

typedef struct CommandBuffer CommandBuffer;

#endif /* end of include guard: ECS_CORE_H */
