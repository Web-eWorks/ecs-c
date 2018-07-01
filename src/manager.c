#include "manager.h"

ComponentType* Manager_GetComponentType(ECS *ecs, const char *type)
{
	// TODO
	return NULL;
}

int Manager_RegisterComponentType(ECS *ecs, ComponentType *type)
{
	//TODO
	return 0;
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
	// TODO
	return NULL;
}

void Manager_DeleteEntity(ECS *ecs, Entity *entity)
{
	// TODO
}
