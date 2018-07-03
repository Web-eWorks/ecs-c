// component.h

#ifndef ECS_COMPONENT_H
#define ECS_COMPONENT_H

#include "ecs.h"

/*
	A component is an arbitrary structure of a certain type. The type of a
	structure is stored as a hash id, referencing a ComponentType entry.

	Two components must not share the same ID.
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
ComponentInfo* ECS_ComponentNew(ECS *ecs, const char *type);

/*
	Get a component by it's id.
*/
ComponentInfo* ECS_ComponentGet(ECS *ecs, hash_t id);

/*
	Destroys a component, removing it from the owning Entity if present, and
	freeing the ComponentInfo.
*/
void ECS_ComponentDelete(ECS *ecs, ComponentInfo *info);

/*
	Returns a string representation of a component, suitable for printing to the
	user.

	The returned string must be freed by the caller.
*/
const char* ECS_ComponentToString(ECS *ecs, ComponentInfo *info);

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
			ecs,
			"MyComponent",
			&MyComponent_New,
			&MyComponent_Delete,
			sizeof(struct MyComponent));

		//alternatively:
		bool ok = REGISTER_COMPONENT(ecs, MyComponent)

*/
bool ECS_ComponentRegisterType(
	ECS *ecs,
	const char *type,
	component_create_func cr_func,
	component_delete_func dl_func,
	size_t size);

/*
	A set of convenience macros for working with components.

	To declare a component, define a struct and call COMPONENT, passing the
	type. This can be done in any order.

		COMPONENT(Name)
		struct Name {};

	Then, you need to define the allocate and delete functions for the
	component.

		void Name_new(Name *comp);
		void Name_free(Name *comp);

	These are declared as static inline void, but the definition only needs
	void.

	Then, in your initialization, instead of ECS_ComponentRegisterType, call:

		bool res = REGISTER_COMPONENT(ecs, Name);

	And you're done!
*/
#define COMPONENT(T) \
	typedef struct T T;

#define COMPONENT_IMPL(T) \
	static inline void T##_new(T *comp); \
	static void T##_cr(Component *p) { return T##_new((T *)p); } \
	static inline void T##_free(T *comp); \
	static void T##_dl(Component *p) { return T##_free((T *)p); } \

#define REGISTER_COMPONENT(ECS, T) \
	(ECS_ComponentRegisterType(ECS, #T, T##_cr, T##_dl, sizeof(T)))

#endif
