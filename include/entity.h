// entity.h

#ifndef ECS_ENTITY_H
#define ECS_ENTITY_H

#include "ecs.h"

struct Entity {
	hash_t id;
	uint16_t carr_num;
	uint16_t carr_cap;
	hash_t *components;
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
	Gets an entity by ID. Returns NULL if no entity was found.
*/
Entity* ECS_EntityGet(ECS *ecs, hash_t id);

/*
	Generate a string representation of an entity, suitable for display to the
	user.

	The returned string must be freed by the caller.
*/
const char* ECS_EntityToString(Entity *entity);

/* -------------------------------------------------------------------------- */

/*
	Creates a new component and adds it to the entity.
*/
Component* ECS_EntityAddComponent(Entity *entity, hash_t type);

/*
	Return a component's data if it is attached to this entity.
*/
Component* ECS_EntityGetComponent(Entity *entity, hash_t type);

/*
	Returns the ComponentID of the specified component attached to this entity.
	Does not check if there is a component of that type, simply returns an ID.
*/
ComponentID ECS_EntityGetComponentID(Entity *entity, hash_t type);

/*
	Remove a component from the entity and free the component's data.
*/
void ECS_EntityDeleteComponent(Entity *entity, hash_t type);

#endif
