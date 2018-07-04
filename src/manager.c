#include "manager.h"

// Get a component type's info.
ComponentType* Manager_GetComponentType(ECS *ecs, const char *type)
{
	assert(ecs && type);

	return ht_get(ecs->cm_types, hash_string(type));
}

// Registers a new component type.
bool Manager_RegisterComponentType(ECS *ecs, ComponentType *type)
{
	assert(ecs && ecs->components && ecs->cm_types && type);

	type = ht_insert(ecs->cm_types, type->type_hash, type);
	if (type == NULL) return false;

	// We store component data with the id of the owning ComponentInfo.
	if (!dyn_alloc(&type->components, 64, sizeof(hash_t) + type->type_size)) {
		ht_delete(ecs->cm_types, type->type_hash);
		return false;
	}

	return true;
}

// Return whether a component type has been registered.
bool Manager_HasComponentType(ECS *ecs, hash_t type)
{
	assert(ecs && ecs->cm_types && type);

	return ht_get(ecs->cm_types, type) ? true : false;
}

ComponentInfo* Manager_CreateComponent(ECS *ecs, ComponentType *type)
{
	assert(ecs && type);

	// Create the ComponentInfo
	hash_t c_id = ecs->_last_component++;
	ComponentInfo *info = ht_insert(ecs->components, c_id, NULL);
	if (!info) return NULL;

	// Setup the members
	info->id = c_id;
	info->type = type->type_hash;
	dynarray_t *carr = &type->components;

	// Allocate the component data
	void *comp = dyn_insert(carr, carr->size, NULL);
	if (!comp) {
		ht_delete(ecs->components, info->id);
		return NULL;
	}

	// Store the id with the data.
	*(hash_t *)comp = info->id;
	// Set the component data pointer.
	comp += sizeof(hash_t);
	info->component = (Component *)comp;

	// Run the component creation function.
	if (type->cr_func) type->cr_func(info->component);

	return info;
}

ComponentInfo* Manager_GetComponent(ECS *ecs, hash_t id)
{
	assert(ecs && id);

	return ht_get(ecs->components, id);
}

void Manager_DeleteComponent(ECS *ecs, ComponentInfo *comp)
{
	assert(ecs && comp && !comp->owner);

	ComponentType *type = ht_get(ecs->cm_types, comp->type);
	if (!type) return;

	// Call the dtor.
	if (type->dl_func) type->dl_func(comp->component);

	// Check that this type actually has components.
	dynarray_t *carr = &type->components;
	if (carr->size == 0) return;

	// Find the index of the component to be deleted.
	int comp_idx = -1;
	for (int idx = 0; idx < carr->size; idx++) {
		hash_t *hash = dyn_get(carr, idx);
		if (hash && *hash == comp->id) {
			comp_idx = idx;
			break;
		}
	}

	// Can't find the component???
	// Should never happen, but can't be too careful.
	if (comp_idx == -1) return;

	// Get the component at the end of the array.
	hash_t *move_hash = dyn_get(carr, -1);
	ComponentInfo *move_info = ht_get(ecs->components, *move_hash);

	// Swap it into the place of the deleted component.
	if (!dyn_swap(carr, comp_idx, -1)) return;
	// And delete the component.
	dyn_delete(carr, -1);

	// Correct the moved component's pointer.
	move_info->component = comp->component;

	// And delete the old component info.
	ht_delete(ecs->components, comp->id);
}

/* -------------------------------------------------------------------------- */

Entity* Manager_CreateEntity(ECS *ecs)
{
	assert(ecs);
	const hash_t ent_id = ecs->_last_entity++;

	Entity *entity = ht_insert(ecs->entities, ent_id, NULL);
	if (entity == NULL) return NULL;

	entity->id = ent_id;
	dyn_alloc(&entity->components, 8, sizeof(hash_t));
	entity->ecs = ecs;

	return entity;
}

Entity* Manager_GetEntity(ECS *ecs, hash_t id)
{
	assert(ecs && id);

	return ht_get(ecs->entities, id);
}

void Manager_DeleteEntity(ECS *ecs, Entity *entity)
{
	assert(ecs && entity);

	for (size_t idx = 0; idx < entity->components.size; idx++) {
		hash_t hash = *(hash_t *)dyn_get(&entity->components, idx);
		ComponentInfo *comp = ECS_EntityGetComponent(entity, hash);
		ECS_EntityRemoveComponent(entity, hash);
		ECS_ComponentDelete(ecs, comp);
	}

	dyn_free(&entity->components);
	ht_delete(ecs->entities, entity->id);
}

/* -------------------------------------------------------------------------- */

bool Manager_RegisterSystem(ECS *ecs, SystemInfo *info)
{
	assert(ecs && ecs->systems && info);

	bool ok = dyn_alloc(&info->ent_queue, 128, sizeof(hash_t));
	if (!ok) return false;

	SystemInfo *_info = ht_insert(ecs->systems, hash_string(info->name), info);

	if (!_info) {
		free((char *)info->name);
		free((char *)info->collection.types);
		dyn_free(&info->ent_queue);
	}

	return _info ? true : false;
}

SystemInfo* Manager_GetSystem(ECS *ecs, const char *name)
{
	assert(ecs && ecs->systems && name);

	return ht_get(ecs->systems, hash_string(name));
}

void Manager_UnregisterSystem(ECS *ecs, const char *name)
{
	assert(ecs && ecs->systems && name);

	SystemInfo *info = ht_get(ecs->systems, hash_string(name));
	if (!info) return;

	EventQueue_Free(info->ev_queue);
	free((char *)info->name);

	ht_delete(ecs->systems, hash_string(name));
}

bool Manager_ShouldSystemQueueEntity(ECS *ecs, SystemInfo *system, Entity *entity)
{
	assert(ecs && system && entity);

	// If we don't want at least one component, we only update the system once.
	if (system->collection.size < 1) return false;

	bool should_queue = true;
	for (size_t idx = 0; idx < system->collection.size; idx++) {
		if (!ECS_EntityGetComponentOfType(entity, system->collection.types[idx], 0)) {
			should_queue = false;
			break;
		}
	}

	return should_queue;
}
