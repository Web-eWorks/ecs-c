// component.c

#include "component.h"
#include "manager.h"

const char* ECS_ComponentToString(ECS *ecs, ComponentID comp)
{
	assert(ecs && ecs->cm_types);
	ComponentType *type = ht_get(ecs->cm_types, comp.type);
	if (!type) return NULL;

	char *str = malloc(strlen(type->type) + 12);
	sprintf(str, "%s (%08x)", type->type, comp.id);

	return str;
}

bool ECS_ComponentRegisterType(ECS *ecs, const ComponentRegistry *reg)
{
	assert(ecs && reg && reg->type);

	const char *type = reg->type;

	if (strlen(type) == 0) {
		printf("Error: cannot register component types with empty names.\n");
		return false;
	}

	if (ECS_HasComponentType(ecs, type)) {
		printf("Error: cannot re-register an already existing type %s.\n", type);
		return false;
	}

	ComponentType c_type = {
		malloc(strlen(type) + 1),
		reg->cr_func, reg->dl_func,
		reg->size,
		hash_string(type)
	};

	strcpy((char *)c_type.type, type);

	if (!Manager_RegisterComponentType(ecs, &c_type)) {
		printf("Error: could not register component type %s.\n", type);
		return false;
	}

	return true;
}

bool ECS_HasComponentType(ECS *ecs, const char *type)
{
	assert(ecs && type);

	return Manager_HasComponentType(ecs, hash_string(type));
}
