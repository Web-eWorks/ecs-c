#include "manager.h"

// Get a component type's info.
ComponentType* Manager_GetComponentType(ECS *ecs, hash_t type)
{
	assert(ecs);

	return ht_get(ecs->cm_types, type);
}

// Registers a new component type.
bool Manager_RegisterComponentType(ECS *ecs, ComponentType *type)
{
	assert(ecs && ecs->components && ecs->cm_types && type);

	type = ht_insert(ecs->cm_types, type->type_hash, type);
	if (type == NULL) return false;

	type->components = mp_init(128, type->type_size);
	if (!type->components) {
		free((char *)type->type);
		ht_delete(ecs->cm_types, type->type_hash);
		return false;
	}

	return true;
}

// Return whether a component type has been registered.
bool Manager_HasComponentType(ECS *ecs, hash_t type)
{
	assert(ecs && ecs->cm_types);

	return ht_get(ecs->cm_types, type) ? true : false;
}

ComponentInfo* Manager_CreateComponent(ECS *ecs, ComponentType *type)
{
	assert(ecs && type);

	// Create the ComponentInfo
	hash_t c_id;
	ComponentInfo *info = ha_insert_free(ecs->components, &c_id, NULL);
	if (!info) return NULL;

	// Setup the members
	info->id = c_id;
	info->type = type->type_hash;

	// Allocate the component data
	info->component = mp_alloc(type->components);
	if (!info->component) {
		ha_delete(ecs->components, info->id);
		return NULL;
	}

	// Zero-initialize the component data,
	memset(info->component, 0, type->type_size);
	// and run the component creation function.
	if (type->cr_func) type->cr_func(info->component);

	return info;
}

ComponentInfo* Manager_GetComponent(ECS *ecs, hash_t id)
{
	assert(ecs);

	return ha_get(ecs->components, id);
}

void Manager_DeleteComponent(ECS *ecs, ComponentInfo *comp)
{
	assert(ecs && comp && !comp->owner);

	// Components must have a valid type.
	ComponentType *type = ht_get(ecs->cm_types, comp->type);
	if (!type) return;

	// Call the dtor.
	if (type->dl_func) type->dl_func(comp->component);

	// Delete the component and it's data.
	mp_free(type->components, comp->component);
	ha_delete(ecs->components, comp->id);
}

/* -------------------------------------------------------------------------- */

Entity* Manager_CreateEntity(ECS *ecs)
{
	assert(ecs);

	hash_t ent_id;
	Entity *entity = ha_insert_free(ecs->entities, &ent_id, NULL);
	if (entity == NULL) return NULL;

	entity->id = ent_id;
	dyn_alloc(&entity->components, 8, sizeof(hash_t));
	entity->ecs = ecs;

	return entity;
}

Entity* Manager_GetEntity(ECS *ecs, hash_t id)
{
	assert(ecs);

	return ha_get(ecs->entities, id);
}

void Manager_DeleteEntity(ECS *ecs, Entity *entity)
{
	assert(ecs && entity);

	while (entity->components.size > 0) {
		hash_t hash = *(hash_t *)dyn_get(&entity->components, 0);
		ComponentInfo *comp = ECS_EntityGetComponent(entity, hash);
		if (!comp) continue;

		// clear the component's owner
		comp->owner = NULL;
		// remove the component from the entity
		dyn_swap(&entity->components, 0, -1);
		dyn_delete(&entity->components, -1);
		// delete the component.
		Manager_DeleteComponent(ecs, comp);
	}

	// Remove the entity from the system's queue.
	HT_FOR(ecs->systems, 0) {
		System *system = ht_get(ecs->systems, idx);
		ht_delete(system->ent_queue, entity->id);
	}

	dyn_free(&entity->components);
	ha_delete(ecs->entities, entity->id);
}

/* -------------------------------------------------------------------------- */

bool Manager_RegisterSystem(ECS *ecs, System *info)
{
	assert(ecs && ecs->systems && info);

	info->ent_queue = ht_alloc(128, sizeof(hash_t));
	if (!info->ent_queue) return false;

	System *_info = ht_insert(ecs->systems, hash_string(info->name), info);

	if (!_info) {
		free((char *)info->name);
		free((char *)info->collection.types);
		free((char *)info->collection.comps);
		ht_free(info->ent_queue);
	}

	return _info ? true : false;
}

System* Manager_GetSystem(ECS *ecs, const char *name)
{
	assert(ecs && ecs->systems && name);

	return ht_get(ecs->systems, hash_string(name));
}

void Manager_UnregisterSystem(ECS *ecs, System *system)
{
	assert(ecs && ecs->systems && system);

	EventQueue_Free(system->ev_queue);
	free((char *)system->name);
	System_DeleteCollection(&system->collection);
	ht_free(system->ent_queue);

	ht_delete(ecs->systems, system->name_hash);
}

void Manager_UpdateCollections(ECS *ecs, Entity *entity)
{
	HT_FOR(ecs->systems, 0) {
		System *system = ht_get(ecs->systems, idx);
		if (Manager_ShouldSystemQueueEntity(ecs, system, entity)) {
			ht_insert(system->ent_queue, entity->id, &entity->id);
		}
		else {
			ht_delete(system->ent_queue, entity->id);
		}
	}
}

bool Manager_ShouldSystemQueueEntity(ECS *ecs, System *system, Entity *entity)
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

void Manager_UpdateSystem(ECS *ecs, System *system, Entity *entity)
{
	assert(ecs && system && system->collection.size > 0 && entity);

	// Systems must have an update function to be registered, and entities don't
	// get in the queue without having all the required components.
	for (size_t idx = 0; idx < system->collection.size; idx++) {
		ComponentInfo *comp = ECS_EntityGetComponentOfType(
			entity, system->collection.types[idx], 0);
		system->collection.comps[idx] = comp;
	}

	system->up_func(entity, system->collection.comps, system->udata);
}

void Manager_SystemEvent(ECS *ecs, System *system, Event *ev)
{
	assert(ecs && system && ev);
	if (!system->ev_func) return;

	system->ev_func(ev, system->udata);
}
