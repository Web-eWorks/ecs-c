#include "manager.h"

int string_arr_to_type(hash_t **dst, const char **src)
{
    if (!dst || !src) return -1;

    size_t idx = 0;
    while(src[idx]) idx++;

    hash_t *ptr = calloc(idx, sizeof(hash_t));
    if (!ptr) return -1;

    *dst = ptr;
    for (size_t _i = 0; _i < idx; _i++) {
        ptr[_i] = hash_string(src[_i]);
    }

    return idx;
}

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

// An entity is literally a 32-bit integer. We drop it in the entity list for
// reasons that need to be re-evaluated.
//
// It might be possible to replace the entity list with a simple gap-spanning
// tracker for free IDs. This would make entity deletion slightly more complex,
// but would likely significantly reduce the amount of complexity and memory
// required to create and manage entities.
//
// This function should not be called from a thread that does not have a lock
// on the entity list.
//
// Due to the time cost of acquiring a lock, locking should be performed in a
// higher-level logic structure, preferably one that handles creating multiple
// entities at once.
Entity Manager_CreateEntity(ECS *ecs)
{
	assert(ecs);

	hash_t entity;
	(void) ha_insert_free(ecs->entities, &entity, NULL);

	return entity;
}

// This function should not be called on a thread that does not have a global lock
// on the ECS.
void Manager_DeleteEntity(ECS *ecs, Entity entity)
{
	assert(ecs);

	// Because we don't keep state on the entity, we iterate through all
	// possible components and delete the ones matching the entity.
	HT_FOR(ecs->cm_types, 0) {
		ComponentType *cm_type = ht_get(ecs->cm_types, idx);
		if (ht_get(cm_type->components, entity))
			Manager_DeleteComponent(ecs, cm_type, entity);
	}

	// Remove the entity from system queues.
	HT_FOR(ecs->systems, 0) {
		System *system = ht_get(ecs->systems, idx);
		ha_delete(system->ent_queue, entity);
	}

	ha_delete(ecs->entities, entity);
}

/* -------------------------------------------------------------------------- */

bool Manager_RegisterSystem(ECS *ecs, System *info)
{
	assert(ecs && ecs->systems && info);

	info->ent_queue = ha_alloc(128, sizeof(hash_t));
    ERR_RET_ZERO(info->ent_queue, "Error creating system entity queue.\n");

	System *_info = ht_insert(ecs->systems, hash_string(info->name), info);
    // TODO: respect system dependencies and queue systems in the most optimal way.
    dyn_insert(&ecs->system_order, ecs->system_order.size, &_info);
    ecs->update_systems_dirty = true;

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

    int idx = dyn_find(&ecs->system_order, &system);
    dyn_remove(&ecs->system_order, idx, false);
    ecs->update_systems_dirty = true;

	EventQueue_Free(system->ev_queue);
	free((char *)system->name);
	ha_free(system->ent_queue);

	ht_delete(ecs->systems, system->name_hash);
}

void Manager_UpdateCollections(ECS *ecs, Entity entity)
{
	HT_FOR(ecs->systems, 0) {
		System *system = ht_get(ecs->systems, idx);
		if (Manager_ShouldSystemQueueEntity(ecs, system, entity)) {
			ha_insert_free(system->ent_queue, NULL, &entity);
		}
		else {
			ha_delete(system->ent_queue, entity);
		}
	}
}

bool Manager_ShouldSystemQueueEntity(ECS *ecs, System *system, Entity entity)
{
	assert(ecs && system);

	// If we don't want at least one component, we only update the system once.
	if (system->archetype->size < 1) return false;

	bool should_queue = true;
	for (size_t idx = 0; idx < system->archetype->size; idx++) {
		ComponentID id = {entity, system->archetype->components[idx]};
		if (!Manager_GetComponentByID(ecs, id)) {
			should_queue = false;
			break;
		}
	}

	return should_queue;
}

void Manager_UpdateSystem(ECS *ecs, System *system, Entity entity)
{
	assert(ecs && system && system->archetype->size > 0);

	const size_t size = system->archetype->size;
	Component *components[size];

	// Systems must have an update function to be registered, and entities don't
	// get in the queue without having all the required components.
	for (size_t idx = 0; idx < size; idx++) {
		ComponentID id = {entity, system->archetype->components[idx]};
		components[idx] = Manager_GetComponentByID(ecs, id);
	}

	system->up_func(entity, components, system->udata);
}

void Manager_SystemEvent(ECS *ecs, System *system, Event *ev)
{
	assert(ecs && system && ev);
	if (!system->ev_func) return;

	system->ev_func(ev, system->udata);
}
