#ifndef ECS_ENTITY_H
#define ECS_ENTITY_H

#include "ecs.h"

struct Entity {
	hash_t id;
	hashtable_t components;
	ECS *ecs;
};

Entity* ECS_EntityNew(ECS *ecs);

void ECS_EntityDelete(Entity* entity);

/* -------------------------------------------------------------------------- */

hash_t ECS_EntityAddComponent(Entity *entity, ComponentInfo *comp);

ComponentInfo* ECS_EntityGetComponent(Entity *entity, hash_t comp);

void ECS_EntityRemoveComponent(Entity *entity, hash_t comp);


#endif