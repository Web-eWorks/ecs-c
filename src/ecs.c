#include "ecs.h"
#include "manager.h"

ECS* ECS_New()
{
	ECS *ecs = malloc(sizeof(ECS));

	ecs->components = ht_alloc(64, sizeof(ComponentInfo));
	ecs->cm_types = ht_alloc(64, sizeof(ComponentType));

	ecs->systems = ht_alloc(64, sizeof(SystemInfo));
	ecs->entities = ht_alloc(256, sizeof(Entity));
	ecs->_last_entity = 1;
	ecs->_last_component = 1;
	ecs->_last_system = 1;

	return ecs;
}

void ECS_Delete(ECS *ecs)
{
	assert(ecs);

	HT_FOR(ecs->entities, 0) {
		Entity *entity = ht_get(ecs->entities, idx);
		if (entity) Manager_DeleteEntity(ecs, entity);
	}
	ht_free(ecs->entities);

	HT_FOR(ecs->systems, 0) {
		SystemInfo *system = ht_get(ecs->systems, idx);
		if (system) Manager_UnregisterSystem(ecs, system->name);
	}
	ht_free(ecs->systems);

	HT_FOR(ecs->components, 0) {
		ComponentInfo *comp = ht_get(ecs->components, idx);
		if (comp) Manager_DeleteComponent(ecs, comp);
	}
	ht_free(ecs->components);

	HT_FOR(ecs->cm_types, 0) {
		ComponentType *type = ht_get(ecs->cm_types, idx);
		if (type) {
			dyn_free(&type->components);
			free((char *)type->type);
			ht_delete(ecs->cm_types, idx);
		}
	}
	ht_free(ecs->cm_types);

	free(ecs);
}
