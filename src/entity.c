#include "entity.h"
#include "manager.h"

Entity* ECS_EntityNew(ECS *ecs)
{
	assert(ecs);
	
	Entity* entity = Manager_CreateEntity(ecs);
	if (entity == NULL) {
		fprintf(stderr, "Error creating entity.\n");
		return NULL;
	}
	
	return entity;
}

void ECS_EntityDelete(Entity *entity)
{
	assert(entity && entity->ecs);
	
	Manager_DeleteEntity(entity->ecs, entity);
	free(entity);
}

hash_t ECS_EntityAddComponent(Entity *entity, ComponentInfo *comp)
{
	assert(entity && entity->ecs && comp);
	
	if (comp->owner != NULL) {
		fprintf(stderr, "Error: component %x is already attached to entity %x.\n",
			comp->id, comp->owner);
	}

	// TODO
	return 0;
}


ComponentInfo* ECS_EntityGetComponent(Entity *entity, hash_t comp)
{
	assert(entity);

	// TODO
	return NULL;
}

void ECS_EntityRemoveComponent(Entity *entity, hash_t comp)
{
	// TODO
	return;
}
