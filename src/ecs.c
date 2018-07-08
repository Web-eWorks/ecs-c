#include "ecs.h"
#include "manager.h"
#include "profile.h"

ECS* ECS_New()
{
	// Default allocation granularity.
	// Each application should perform their own testing, but these are the best
	// values found for the test harness application.
	const ECS_AllocInfo alloc = {
		// Components and entities
		256, 256,
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
	dyn_alloc(&ecs->update_systems, alloc->systems, sizeof(SystemInfo *));
	ecs->_last_system = 1;

	ecs->cm_types = ht_alloc(alloc->cm_types, sizeof(ComponentType));
	ecs->alloc_info = *alloc;

	return ecs;
}

void ECS_Delete(ECS *ecs)
{
	assert(ecs);

	// Deleting entities will delete all attached components, which make up
	// the extreme majority of all components.
	HT_FOR(ecs->entities, 0) {
		Entity *entity = ht_get(ecs->entities, idx);
		if (entity) ECS_EntityDelete(entity);
	}
	ht_free(ecs->entities);

	// There are only a handful of component deletions to perform at this point.
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
	PERF_START();

	// Iterate systems.
	HT_FOR(ecs->systems, 0) {
		SystemInfo *system = ht_get(ecs->systems, idx);
		// queue the system for updates
		dyn_insert(&ecs->update_systems, ecs->update_systems.size, &system);
	}

	// TODO: allow systems to define their order of execution.
	// TODO: handle circular referencing in the systems.
	// TODO: maybe make system ordering an issue of the caller?

	PERF_PRINT_MS("UpdateBegin");

	return true;
}

static void ECS_UpdateSystem(ECS *ecs, SystemInfo *system)
{
	PERF_START();

	// Update the system with all entities that have the correct components.
	if (system->collection.size > 0) {
		HT_FOR(system->ent_queue, 0) {
			hash_t *ent_hash = ht_get(system->ent_queue, idx);
			Entity *entity = ht_get(ecs->entities, *ent_hash);
			if (!entity) continue;

			Manager_UpdateSystem(ecs, system, entity);
		}
	}
	// If the system operate on components, we only update it once.
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
	PERF_PRINT_CUSTOM(TIME_FACTOR_MS, "UpdateSystem: %s took %.2fms to complete.\n", system->name);
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
	PERF_START();

	PERF_PRINT_MS("UpdateEnd");
}
