#include "manager.h"

ComponentArray* Manager_CreateComponentArray(ECS *ecs, ComponentType *type, size_t cap)
{
	ComponentArray *arr = ht_insert(ecs->components, type->type_hash, NULL);
	if (!arr) return NULL;
	
	arr->size = type->type_size;
	arr->count = 0;
	
	arr->capacity = cap;
	arr->ptr = calloc(arr->capacity, arr->size);
	if (!arr->ptr) return NULL;
	
	return arr;
}


/* -------------------------------------------------------------------------- */

// Get a component type's info.
ComponentType* Manager_GetComponentType(ECS *ecs, const char *type)
{
	assert(ecs && type);
	
	hash_t type_hash = hash_string(type);
	return ht_get(ecs->cm_types, type_hash);
}

// Registers a new component type.
bool Manager_RegisterComponentType(ECS *ecs, ComponentType *type)
{
	assert(ecs && ecs->components && ecs->cm_types && type);

	type = ht_insert(ecs->cm_types, type->type_hash, type);
	if (type == NULL) return false;

	ComponentArray *arr = Manager_CreateComponentArray(ecs, type, 64);
	if (arr == NULL) {
		ht_delete(ecs->cm_types, type->type_hash);
		return false;
	}
	
	type->components = arr;
	return true;
}

Component* Manager_CreateComponent(ECS *ecs, ComponentType *type)
{
	//TODO
	return NULL;
}

void Manager_DeleteComponent(ECS *ecs, Component *comp)
{
	//TODO
}

/* -------------------------------------------------------------------------- */

Entity* Manager_CreateEntity(ECS *ecs)
{
	assert(ecs);
	const hash_t ent_id = ++ecs->_last_entity;
	
	Entity *entity = ht_insert(ecs->entities, ent_id, NULL);
	if (entity == NULL) return NULL;
	
	entity->id = ent_id;
	entity->components = ht_alloc(32, sizeof(ComponentInfo));
	entity->ecs = ecs;
	
	return entity;
}

void Manager_DeleteEntity(ECS *ecs, Entity *entity)
{
	assert(ecs && entity);
	
	// TODO: free the entity's components
	
	ht_delete(ecs->entities, entity->id);
	ht_free(entity->components);
	free(entity);
}
