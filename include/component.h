// component.h

#ifndef ECS_COMPONENT_H
#define ECS_COMPONENT_H

#include "ecs.h"

/*
	Returns a string representation of a component, suitable for printing to the
	user.

	The returned string must be freed by the caller.
*/
const char* ECS_ComponentToString(ECS *ecs, ComponentID id);

/*
	Custom component initialization function.

	The function is passed a pointer to pre-allocated memory, and should only
	allocate memory where necessary for the component's members.
*/
typedef void (*component_create_func)(Component*);

/*
	Custom component deletion function.

	This function is passed a pointer to memory that will be freed by the ECS,
	and should only perform deletion of the component's members where necessary.
*/
typedef void (*component_delete_func)(Component*);

/*
	Defines how components are stored.

	ComponentStorageNormal:
		A 1:1 mapping between components and their data. Component data creation
		and deletion is manually controlled by instantiation code.
	ComponentStorageFlyweight:
		Components follow the flyweight pattern. Multiple components reference the
		same component data. Useful for read-only components.
	ComponentStorageNone:
		Used for empty components that are just used as a signalling mechanism.
		Component creation and deletion functions will not be called.
*/
typedef enum {
	ComponentStorageNormal,
    ComponentStorageFlyweight,
    ComponentStorageNone
} ComponentStorage;

/*
	Information about a component type.

	When using
*/
typedef struct {
	const char *type;
	size_t size;
	ComponentStorage storage;

	component_create_func cr_func;
	component_delete_func dl_func;
} ComponentRegistry;

/*
	Registers a new type of component.

	@param type: the typename of the new type
	@param reg: the component's registry info
	@returns: true if successful, false otherwise

	Example usage:

		ComponentRegistry reg = {
			"MyComponent",
			sizeof(MyComponent),
			ComponentStorageNormal,
			&MyComponent_New,
			&MyComponent_Delete
		};
		bool ok = ECS_ComponentRegisterType(ecs, &reg);
*/
bool ECS_ComponentRegisterType(ECS *ecs, const ComponentRegistry *registry);

/*
	Returns whether a component type has already been registered with that
	name.
*/
bool ECS_HasComponentType(ECS *ecs, const char *type);

/*
	A set of convenience macros for working with components.

	To declare a component, define a struct and call COMPONENT, passing the
	type. This can be done in any order.

		COMPONENT(Name)
		struct Name {};

	Then, you need to define the allocate and delete functions for the
	component.

		COMPONENT_IMPL(Name, StorageType)
		void Name_new(Name *comp);
		void Name_free(Name *comp);

	Components using the ComponentStorageNone type should call COMPONENT_IMPL_NONE
	instead, as this doesn't define unused component handler functions.

	Then, in your initialization, instead of ECS_ComponentRegisterType, call:

		bool res = REGISTER_COMPONENT(ecs, Name);

	And you're done!
*/
#define COMPONENT(T) \
	typedef struct T T;

#define COMPONENT_IMPL(T, Storage) \
	static inline void T##_new(T *comp); \
	static void T##_cr(Component *p) { return T##_new((T *)p); } \
	static inline void T##_free(T *comp); \
	static void T##_dl(Component *p) { return T##_free((T *)p); } \
	const ComponentRegistry T##_reg  = { \
		#T, sizeof(T), Storage, \
		T##_cr, T##_dl \
	};

#define COMPONENT_IMPL_NONE(T) \
	const ComponentRegistry T##_reg { \
		#T, sizeof(T), ComponentStorageNone, \
		NULL, NULL \
	};

#define REGISTER_COMPONENT(ECS, T) \
	(ECS_ComponentRegisterType(ECS, &T##_reg))

#define COMPONENT_ID(T) hash_string(#T)

#endif
