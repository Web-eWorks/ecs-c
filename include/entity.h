// entity.h

#ifndef ECS_ENTITY_H
#define ECS_ENTITY_H

#include "ecs.h"

struct Entity {
	hash_t id;
	dynarray_t components;
	ECS *ecs;
};

/*
	Creates a new entity.
*/
Entity* ECS_EntityNew(ECS *ecs);

/*
	Deletes an entity. All components attached to the entity are also freed.
*/
void ECS_EntityDelete(Entity* entity);

/*
	Generate a string representation of an entity, suitable for display to the
	user.

	The returned string must be freed by the caller.
*/
const char* ECS_EntityToString(Entity *entity);

/* -------------------------------------------------------------------------- */

/*
	Adds a component to the entity. If the component is already attached to
	another entity, the function fails.
*/
bool ECS_EntityAddComponent(Entity *entity, ComponentInfo *comp);

/*
	Return a component's data if it is attached to this entity.
*/
ComponentInfo* ECS_EntityGetComponent(Entity *entity, hash_t comp);

/*
	Remove a component from the entity. Does not free the component.
*/
void ECS_EntityRemoveComponent(Entity *entity, hash_t comp);

#endif
