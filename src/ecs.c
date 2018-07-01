#include "ecs.h"
#include "manager.h"

ECS* ECS_New()
{
	ECS *ecs = malloc(sizeof(ECS));

	// TODO: initialization
	return ecs;
}

void ECS_Delete(ECS *ecs)
{
	assert(ecs);
	
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