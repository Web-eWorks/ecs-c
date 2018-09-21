// ecs.h

/*
	Most interaction with the 'global' portion of the ECS will be taking place
	using these functions.
*/

#ifndef ECS_MAIN_H
#define ECS_MAIN_H

#include "core.h"

#include "event.h"
#include "component.h"
#include "entity.h"
#include "system.h"

/*
    The performance of the ECS is defined by the contiguous nature of its data
    storage.

    The default values provide a fairly good tradeoff between memory
    fragmentation and consumption, but an application may wish to pass its own
    values for these allocations.
*/
typedef struct {
    size_t components;
    size_t entities;
    size_t systems;
    size_t cm_types;

    size_t system_entities;
} ECS_AllocInfo;

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
    Run an update on all systems that need it.
*/
void ECS_Update(ECS *ecs);

CommandBuffer* CommandBuffer_New(ECS *ecs);
void CommandBuffer_Delete(CommandBuffer *buff);

hash_t CommandBuffer_CreateEntity(CommandBuffer *buff);
void CommandBuffer_DeleteEntity(CommandBuffer *buff, hash_t entity);

void CommandBuffer_AddComponent(CommandBuffer *buff, hash_t entity, hash_t component);
void CommandBuffer_RemoveComponent(CommandBuffer *buff, hash_t entity, hash_t component);

#endif
