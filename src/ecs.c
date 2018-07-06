#include "ecs.h"
#include "manager.h"
#include "profile.h"

ECS* ECS_New()
{
	// Default allocation granularity
	const ECS_AllocInfo alloc = {
		// Components and entities
		1024, 1024,
		// Systems and component types
		64, 64,
		// Number of components / entity
		16,
		// Number of entities systems will operate on.
		256
	};
	return ECS_CustomNew(&alloc);
}


ECS* ECS_CustomNew(const ECS_AllocInfo *alloc)
{
	assert(alloc);

	ECS *ecs = malloc(sizeof(ECS));

	ecs->components = ht_alloc(alloc->components, sizeof(ComponentInfo));
	ecs->_last_component = 1;

	ecs->entities = ht_alloc(alloc->entities, sizeof(Entity));
	ecs->_last_entity = 1;

	ecs->systems = ht_alloc(alloc->systems, sizeof(SystemInfo));
	dyn_alloc(&ecs->update_systems, alloc->systems, sizeof(hash_t));
	ecs->_last_system = 1;

	ecs->cm_types = ht_alloc(alloc->cm_types, sizeof(ComponentType));
	ecs->alloc_info = *alloc;

	return ecs;
}

void ECS_Delete(ECS *ecs)
{
	assert(ecs);

	HT_FOR(ecs->entities, 0) {
		Entity *entity = ht_get(ecs->entities, idx);
		if (entity) ECS_EntityDelete(entity);
	}
	ht_free(ecs->entities);

	// In most usage scenarios, deleting entities will delete all attached
	// components, which make up the extreme majority of all components.
	HT_FOR(ecs->components, 0) {
		ComponentInfo *comp = ht_get(ecs->components, idx);
		if (comp) ECS_ComponentDelete(ecs, comp);
	}
	ht_free(ecs->components);

	HT_FOR(ecs->systems, 0) {
		SystemInfo *system = ht_get(ecs->systems, idx);
		if (system) Manager_UnregisterSystem(ecs, system);
	}
	ht_free(ecs->systems);
	dyn_free(&ecs->update_systems);

	HT_FOR(ecs->cm_types, 0) {
		ComponentType *type = ht_get(ecs->cm_types, idx);
		if (type) {
			mp_destroy(type->components);
			free((char *)type->type);
			ht_delete(ecs->cm_types, idx);
		}
	}
	ht_free(ecs->cm_types);

	free(ecs);
}

#include <time.h>

// Create entity queues for each system, resolve the order of systems, etc.
bool ECS_UpdateBegin(ECS *ecs)
{
	// Iterate systems.
	HT_FOR(ecs->systems, 0) {
		SystemInfo *system = ht_get(ecs->systems, idx);
		// queue the system for updates
		dyn_insert(&ecs->update_systems, ecs->update_systems.size, &idx);
		// clear the system's entity queue (should already be clear).
		while (system->ent_queue.size > 0) dyn_delete(&system->ent_queue, -1);
	}

	// TODO: allow systems to define their order of execution.
	// TODO: handle circular referencing in the systems.
	// TODO: maybe make system ordering an issue of the caller?

	HT_FOR(ecs->entities, 0) {
		// TODO: find an algorithm for this that isn't O(NxMxO), or at least do it less often.
		// Maybe when an entity has changed components, mark it dirty?
		Entity *entity = ht_get(ecs->entities, idx);

		HT_FOR(ecs->systems, 0) {
			SystemInfo *system = ht_get(ecs->systems, idx);

			// Check if we should queue the entity
			if (Manager_ShouldSystemQueueEntity(ecs, system, entity)) {
				dyn_insert(&system->ent_queue, system->ent_queue.size, &entity->id);
			}
		}
	}

	return true;
}

static void ECS_UpdateSystem(ECS *ecs, SystemInfo *system)
{
	// If the system wants components, go through all the queued entities
	// and call the update function on each set of components.
	if (system->collection.size > 0) {
		for (size_t idx = 0; idx < system->ent_queue.size; idx++) {
			hash_t ent_hash = *(hash_t *)dyn_get(&system->ent_queue, idx);

			Entity *entity = ht_get(ecs->entities, ent_hash);
			if (!entity) continue;

			Manager_UpdateSystem(ecs, system, entity);
		}
	}
	// If the system doesn't want any components, we only update it once.
	else {
		system->up_func(NULL, system->udata);
	}

	if (system->ev_func) {
		Event *ev = NULL;
		while ((ev = EventQueue_Peek(system->ev_queue, 0)) != NULL) {
			Manager_SystemEvent(ecs, system, ev);
			EventQueue_Pop(system->ev_queue, NULL);
		}
	}
}

//
bool ECS_UpdateSystems(ECS *ecs)
{
	HT_FOR(ecs->systems, 0) {
		SystemInfo *system = ht_get(ecs->systems, idx);
		ECS_UpdateSystem(ecs, system);
	}

	return true;
}

void ECS_UpdateEnd(ECS *ecs)
{

}
