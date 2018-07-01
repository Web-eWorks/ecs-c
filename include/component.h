// component.h

#ifndef ECS_COMPONENT_H
#define ECS_COMPONENT_H

#include "ecs.h"

/*
	A component is an arbitrary structure of a certain type. The type of a
	structure is stored as a hash id, referencing a ComponentType entry.
*/
struct ComponentInfo {
	hash_t id;
	hash_t type;
	Component *component;
	Entity *owner;
};

/*
	Initializes a new component, sets up the ComponentInfo, and returns a
	pointer to the new component.
*/
Component* ECS_ComponentNew(ECS *ecs, const char *type, ComponentInfo *info);

/*
	Destroys a component, removing it from the owning Entity if present, and
	freeing the ComponentInfo.
*/
void ECS_ComponentDelete(ECS *ecs, ComponentInfo *info);


/*
	Functions create and delete custom components.
	
	These functions are passed a pointer to the memory allocated for the
	components by the ECS and should not perform any allocation of their own.
	
	Allocation is handled internally for data locality and performance reasons.
*/
typedef void (*component_create_func)(Component*);
typedef void (*component_delete_func)(Component*);

/*
	Registers a new type of component.

	@param type: the typename of the new type
	@param cr_func: the component data's creation function
	@param dl_func: the component data's deletion function
	@param size: the size of the component's data.
	@returns: true if successful, false otherwise
	
	Example usage:
	
		bool ok = ECS_ComponentRegisterType(
			"MyComponent",
			&MyComponent_New,
			&MyComponent_Delete,
			sizeof(struct MyComponent));
*/
int ECS_ComponentRegisterType(
	ECS *ecs,
	const char *type,
	component_create_func cr_func,
	component_delete_func dl_func,
	size_t size);

#endif