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
}

const char* ECS_EntityToString(Entity *entity)
{
	assert(entity && entity->ecs);

	char *str = malloc(18);
	sprintf(str, "Entity (%08x)", entity->id);

	return str;
}

bool ECS_EntityAddComponent(Entity *entity, ComponentInfo *comp)
{
	assert(entity && entity->ecs && comp);

	if (comp->owner != NULL) {
		fprintf(stderr, "Error: component %x is already attached to entity %x.\n",
			comp->id, comp->owner);
		return false;
	}

	comp->owner = entity;
	dyn_insert(&entity->components, entity->components.size, &comp->id);

	return true;
}

ComponentInfo* ECS_EntityGetComponent(Entity *entity, hash_t hash)
{
	assert(entity && entity->ecs && hash);

	ComponentInfo *comp = Manager_GetComponent(entity->ecs, hash);
	if (!comp || comp->owner != entity) return NULL;

	return comp;
}

void ECS_EntityRemoveComponent(Entity *entity, hash_t hash)
{
	assert(entity && entity->ecs && hash);

	ComponentInfo *comp = ECS_EntityGetComponent(entity, hash);
	if (!comp) return;

	comp->owner = NULL;

	int idx = dyn_find(&entity->components, &hash);
	// should never happen.
	if (idx == -1) {
		fprintf(stderr, "Error: entity %x does not have component %x, even though it should!\n",
			entity->id, comp->id);
		return;
	}

	dyn_swap(&entity->components, idx, -1);
	dyn_delete(&entity->components, -1);
}
