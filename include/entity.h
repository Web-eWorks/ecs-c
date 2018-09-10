// entity.h

#ifndef ECS_ENTITY_H
#define ECS_ENTITY_H

#include "ecs.h"

/*
	Creates a new entity with a set of components.
*/
Entity ECS_EntityNew(ECS *ecs, EntityArchetype *archetype);

/*
	Deletes an entity. All components attached to the entity are also freed.
*/
void ECS_EntityDelete(ECS *ecs, Entity entity);

/*
	Checks if the given entity exists.
*/
bool ECS_EntityExists(ECS *ecs, Entity entity);

/*
	Generate a string representation of an entity, suitable for display to the
	user.

	The returned string must be freed by the caller.
*/
const char* ECS_EntityToString(Entity entity);

/* -------------------------------------------------------------------------- */

/*
	Creates a new component and adds it to the entity.
*/
Component* ECS_EntityAddComponent(ECS *ecs, Entity entity, hash_t type);

/*
	Return a component's data if it is attached to this entity.
*/
Component* ECS_EntityGetComponent(ECS *ecs, Entity entity, hash_t type);

/*
	Returns the ComponentID of the specified component attached to this entity.
	Does not check if there is a component of that type, simply returns an ID.
*/
inline ComponentID ECS_EntityGetComponentID(Entity entity, hash_t type)
{
    ComponentID id = { entity, type };
    return id;
}

/*
	Remove a component from the entity and free the component's data.
*/
void ECS_EntityDeleteComponent(ECS *ecs, Entity entity, hash_t type);

/* -------------------------------------------------------------------------- */

/*
	Registers and creates an entity archetype.
*/
EntityArchetype* ECS_EntityRegisterArchetype(ECS *ecs, const char *name, const char** components);

#endif
