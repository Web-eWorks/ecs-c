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
	assert(ecs && ecs->cm_types && type);

	type = ht_insert(ecs->cm_types, type->type_hash, type);
	if (type == NULL) return false;

	type->components = ht_alloc(ecs->alloc_info.components, type->type_size);
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

Component* Manager_CreateComponent(ECS *ecs, ComponentType *type, hash_t id)
{
	assert(ecs && type);

	// Create the component
	Component *comp = ht_insert(type->components, id, NULL);
	if (!comp) return NULL;

	// And run the creation function.
	if (type->cr_func) type->cr_func(comp);

	return comp;
}

Component* Manager_GetComponent(ECS *ecs, ComponentType *type, hash_t id)
{
	assert(ecs);

	return ht_get(type->components, id);
}

Component* Manager_GetComponentByID(ECS *ecs, ComponentID id)
{
	assert(ecs);

	ComponentType *cm_type = ht_get(ecs->cm_types, id.type);
	if (!cm_type) return NULL;

	return ht_get(cm_type->components, id.id);
}

void Manager_DeleteComponent(ECS *ecs, ComponentType *type, hash_t id)
{
	assert(ecs && type && type->components);

	Component *comp = ht_get(type->components, id);
	if (!comp) return;

	// Call the dtor.
	if (type->dl_func) type->dl_func(comp);

	// Delete the component and it's data.
	ht_delete(type->components, id);
}

/* -------------------------------------------------------------------------- */

Entity* Manager_CreateEntity(ECS *ecs)
{
	assert(ecs);

	hash_t ent_id;
	Entity *entity = ha_insert_free(ecs->entities, &ent_id, NULL);
	if (entity == NULL) return NULL;

	entity->id = ent_id;
	entity->carr_num = 0;
	entity->carr_cap = ecs->alloc_info.entity_components;
	entity->components = calloc(entity->carr_cap, sizeof(hash_t));
	if (!entity->components) ha_delete(ecs->entities, ent_id);
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

	for (size_t idx = 0; idx < entity->carr_num; idx++) {
		ComponentType *cm_type = ht_get(ecs->cm_types, entity->components[idx]);
		if (!cm_type) continue;
		Manager_DeleteComponent(ecs, cm_type, entity->id);
	}

	// Remove the entity from the system's queue.
	HT_FOR(ecs->systems, 0) {
		System *system = ht_get(ecs->systems, idx);
		ha_delete(system->ent_queue, entity->id);
	}

	free(entity->components);
	ha_delete(ecs->entities, entity->id);
}

/* -------------------------------------------------------------------------- */

bool Manager_RegisterSystem(ECS *ecs, System *info)
{
	assert(ecs && ecs->systems && info);

	info->ent_queue = ha_alloc(128, sizeof(hash_t));
	if (!info->ent_queue) return false;

	System *_info = ht_insert(ecs->systems, hash_string(info->name), info);

	if (!_info) {
		free((char *)info->name);
		free((char *)info->collection.types);
		free((char *)info->collection.comps);
		ha_free(info->ent_queue);
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
	ha_free(system->ent_queue);

	ht_delete(ecs->systems, system->name_hash);
}

void Manager_UpdateCollections(ECS *ecs, Entity *entity)
{
	HT_FOR(ecs->systems, 0) {
		System *system = ht_get(ecs->systems, idx);
		if (Manager_ShouldSystemQueueEntity(ecs, system, entity)) {
			ha_insert_free(system->ent_queue, NULL, &entity->id);
		}
		else {
			ha_delete(system->ent_queue, entity->id);
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
		ComponentID id = {entity->id, system->collection.types[idx]};
		if (!Manager_GetComponentByID(ecs, id)) {
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
		ComponentID id = {entity->id, system->collection.types[idx]};
		system->collection.comps[idx] = Manager_GetComponentByID(ecs, id);
	}

	system->up_func(entity, system->collection.comps, system->udata);
}

void Manager_SystemEvent(ECS *ecs, System *system, Event *ev)
{
	assert(ecs && system && ev);
	if (!system->ev_func) return;

	system->ev_func(ev, system->udata);
}
