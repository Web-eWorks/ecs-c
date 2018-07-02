#include "ecs.h"
#include "manager.h"

ECS* ECS_New()
{
	ECS *ecs = malloc(sizeof(ECS));

	ecs->components = ht_alloc(64, sizeof(ComponentInfo));
	ecs->cm_types = ht_alloc(64, sizeof(ComponentType));

	// TODO
	ecs->systems = ht_alloc(64, sizeof(void*));
	ecs->entities = ht_alloc(256, sizeof(void*));
	ecs->_last_entity = 1;
	ecs->_last_component = 1;
	ecs->_last_system = 1;

	// TODO: initialization
	return ecs;
}

void ECS_Delete(ECS *ecs)
{
	assert(ecs);
	
	// TODO: free the individual components, types, entities, and systems.
	
	ht_free(ecs->components);
	ht_free(ecs->cm_types);
	ht_free(ecs->systems);
	ht_free(ecs->entities);
	
	free(ecs);
	// TODO
}

int ECS_AddEntity(ECS *ecs, Entity *entity)
{
	assert(ecs && entity);
	
	if (entity->ecs != NULL) {
		fprintf(stderr, "Error: entity %x is already attached to another ECS.\n",
			entity->id);
		return 0;
	}
	
	// TODO
	return 0;
}

void ECS_RemoveEntity(ECS *ecs, Entity *entity)
{
	assert(ecs && entity);
	
	if (entity->ecs != ecs) {
		fprintf(stderr, "Error: entity %x is not attached to this ECS.\n",
			entity->id);
		return;
	}
	
	// TODO
}