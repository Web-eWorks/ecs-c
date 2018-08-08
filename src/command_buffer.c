// command_buffer.c

#include "manager.h"

CommandBuffer* CommandBuffer_New(ECS *ecs)
{
    hash_t id;
    ECS_ATOMIC(ecs, CommandBuffer *buff = ha_insert_free(ecs->buffers, &id, NULL));
    if (!buff) return NULL;

    buff->ecs = ecs;
    buff->id = id;
    buff->last_entity = 0;
    dyn_alloc(&buff->commands, 16, sizeof(Command));
    pthread_mutex_init(&buff->mutex, NULL);

    return buff;
}

void CommandBuffer_Delete(CommandBuffer *buff)
{
    if (!buff) return;
    ECS *ecs = buff->ecs;

    ECS_LOCK(ecs);
    pthread_mutex_destroy(&buff->mutex);
    dyn_free(&buff->commands);
    ha_delete(ecs->buffers, buff->id);
    ECS_UNLOCK(ecs);
}

static inline void insert(CommandBuffer *buff, Command *cm)
{
    CB_ATOMIC(buff, dyn_insert(&buff->commands, buff->commands.size, cm));
}

hash_t CommandBuffer_CreateEntity(CommandBuffer *buff)
{
    assert(buff);

    Command cm = { CMD_EntityCreate, {buff->last_entity++, 0} };

    insert(buff, &cm);
    return cm.data[0];
}

void CommandBuffer_AddComponent(CommandBuffer *buff, hash_t entity, hash_t component)
{
    assert(buff);

    Command cm = { CMD_ComponentAttach, {entity, component} };
    insert(buff, &cm);
}

void CommandBuffer_RemoveComponent(CommandBuffer *buff, hash_t entity, hash_t component)
{
    assert(buff);

    Command cm = { CMD_ComponentDetach, {entity, component} };
    insert(buff, &cm);
}

void CommandBuffer_DeleteEntity(CommandBuffer *buff, hash_t entity)
{
    assert(buff);
    Command cm = { CMD_EntityDelete, {entity, 0} };
    insert(buff, &cm);
}
