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

Entity* ECS_EntityGet(ECS *ecs, hash_t id)
{
	assert(ecs);

	Entity *entity = ha_get(ecs->entities, id);
	return entity;
}

const char* ECS_EntityToString(Entity *entity)
{
	assert(entity && entity->ecs);

	char *str = malloc(24);
	sprintf(str, "Entity (%08x)", entity->id);

	return str;
}

#define GET_TYPE(ecs, type, ret) ComponentType *cm_type = ht_get(ecs->cm_types, type); \
	if (!cm_type) { \
		ECS_ERROR(ecs, "Unknown component type %x.", type); \
		return ret; \
	}

Component* ECS_EntityAddComponent(Entity *entity, hash_t type)
{
	assert(entity && entity->ecs);

	ECS *ecs = entity->ecs;
	GET_TYPE(ecs, type, NULL);

	// If we already have a component on the entity, return it.
	Component *comp = ht_get(cm_type->components, entity->id);
	if (comp) return comp;

	// Otherwise, create the new component.
	comp = Manager_CreateComponent(ecs, cm_type, entity->id);
	dyn_insert(&entity->components, entity->components.size, &type);

	// Update systems' entity queues.
	// We do this at the AddComponent / RemoveComponent step to gain performance.
	// Adding components to entities takes about 0.5x more time, but it gains an
	// immense amount of performance on the update step.
	Manager_UpdateCollections(entity->ecs, entity);

	return comp;
}

Component* ECS_EntityGetComponent(Entity *entity, hash_t type)
{
	assert(entity && entity->ecs);

	GET_TYPE(entity->ecs, type, NULL);
	Component *comp = ht_get(cm_type->components, entity->id);

	return comp;
}

ComponentID ECS_EntityGetComponentID(Entity *entity, hash_t type)
{
	assert(entity);

	ComponentID id = {entity->id, type};
	return id;
}

void ECS_EntityRemoveComponent(Entity *entity, hash_t type)
{
	assert(entity && entity->ecs);

	GET_TYPE(entity->ecs, type,);

	int idx = dyn_find(&entity->components, &type);
	if (idx < 0) return;

	Manager_DeleteComponent(entity->ecs, cm_type, entity->id);

	dyn_swap(&entity->components, idx, -1);
	dyn_delete(&entity->components, -1);

	Manager_UpdateCollections(entity->ecs, entity);
}
