// core.h

#ifndef ECS_CORE_H
#define ECS_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <pthread.h>

#include "hash.h"
#include "hashtable.h"
#include "hasharray.h"
#include "hashset.h"
#include "dynarray.h"

/*
	Forward declarations.
*/

typedef struct Event Event;

/*
    A component is an encapsulation of a set of data, usually related to a
    single purpose. Components are implemented as black-box data initialized
    and managed by external code, but allocated and stored by the ECS.

    Components are stored in arrays of type, indexed by their owning entity's ID.
*/
typedef void Component;

/*
    A simple way to refer to components.
*/
typedef struct {
	hash_t id;
	hash_t type;
} ComponentID;

/*
    The basic unit of objects. Each entity may be associated with one or more
    components.
*/
typedef hash_t Entity;

/*
    Information about a collection of components, called an Archetype. Systems
    operate on these, and they are used as a fast way to create entities with a
    pre-defined component structure.
*/
typedef struct EntityArchetype EntityArchetype;

/*
    An encapsulation of update code.
*/
typedef struct System System;

/*
    The core datastructure of the ECS.
*/
typedef struct ECS ECS;

typedef struct CommandBuffer CommandBuffer;

#endif /* end of include guard: ECS_CORE_H */
