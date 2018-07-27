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

CommandBuffer* CommandBuffer_New(ECS *ecs);
void CommandBuffer_Delete(CommandBuffer *buff);

hash_t CommandBuffer_CreateEntity(CommandBuffer *buff);
void CommandBuffer_DeleteEntity(CommandBuffer *buff, hash_t entity);

void CommandBuffer_AddComponent(CommandBuffer *buff, hash_t entity, hash_t component);
void CommandBuffer_RemoveComponent(CommandBuffer *buff, hash_t entity, hash_t component);

#endif
