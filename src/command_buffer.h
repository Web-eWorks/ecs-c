// command_buffer.h

#include "core.h"

typedef enum {
    CMD_EntityCreate,
    CMD_ComponentAttach,
    CMD_ComponentDetach,
    CMD_EntityDelete
} Command_T;

typedef struct {
    Command_T type;
    hash_t data[2];
} Command;

struct CommandBuffer {
	ECS *ecs;
    hash_t id;
    hash_t last_entity;
    pthread_mutex_t mutex;
    dynarray_t commands;
};

#define CB_LOCK(cb) pthread_mutex_lock(&cb->mutex)
#define CB_UNLOCK(cb) pthread_mutex_unlock(&cb->mutex)
#define CB_ATOMIC(cb, op) CB_LOCK(cb); op; CB_UNLOCK(cb)
