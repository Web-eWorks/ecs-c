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
			comp->id, comp->owner->id);
		return false;
	}

	comp->owner = entity;
	dyn_insert(&entity->components, entity->components.size, &comp->id);
	// We do this at the AddComponent / RemoveComponent step to gain performance.
	// Adding components to entities takes about 1.5x the time, but it gains an immense
	// amount of performance on the update step.
	Manager_UpdateCollections(entity->ecs, entity);

	return true;
}

ComponentInfo* ECS_EntityGetComponent(Entity *entity, hash_t hash)
{
	assert(entity && entity->ecs);

	ComponentInfo *comp = Manager_GetComponent(entity->ecs, hash);
	if (!comp || comp->owner != entity) return NULL;

	return comp;
}

ComponentInfo* ECS_EntityGetComponentOfType(Entity *entity, hash_t type, size_t idx)
{
	assert(entity && entity->ecs);

	size_t found_count = 0;
	size_t components = entity->components.size;
	for (size_t idx = 0; idx < components; idx++) {
		hash_t *hash = dyn_get(&entity->components, idx);
		ComponentInfo *comp = Manager_GetComponent(entity->ecs, *hash);
		if (!comp || comp->type != type) continue;
		if (found_count == idx) return comp;
		else found_count++;
	}

	return NULL;
}

void ECS_EntityRemoveComponent(Entity *entity, hash_t hash)
{
	assert(entity && entity->ecs);

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

	Manager_UpdateCollections(entity->ecs, entity);
}
