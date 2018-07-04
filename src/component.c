// component.c

#include "component.h"
#include "manager.h"

ComponentInfo* ECS_ComponentNew(ECS *ecs, const char *type)
{
	assert(ecs && type);

	ComponentType* comp_type = Manager_GetComponentType(ecs, type);
	if (comp_type == NULL) {
		fprintf(stderr, "Error: no such component type %s.\n", type);
		return NULL;
	}

	ComponentInfo *comp = Manager_CreateComponent(ecs, comp_type);
	if (comp == NULL) {
		fprintf(stderr, "Error: could not create component of type %s.\n", type);
		return NULL;
	}

	return comp;
}

ComponentInfo* ECS_ComponentGet(ECS *ecs, hash_t id)
{
	assert(ecs && id);

	return Manager_GetComponent(ecs, id);
}

void ECS_ComponentDelete(ECS *ecs, ComponentInfo *info)
{
	assert(ecs && info);

	if (info->owner != NULL) {
		ECS_EntityRemoveComponent(info->owner, info->id);
	}

	Manager_DeleteComponent(ecs, info);
}

const char* ECS_ComponentToString(ECS *ecs, ComponentInfo *comp)
{
	assert(ecs && ecs->cm_types && comp);
	ComponentType *type = ht_get(ecs->cm_types, comp->type);
	if (!type) return NULL;

	char *str = malloc(strlen(type->type) + 12);
	sprintf(str, "%s (%08x)", type->type, comp->id);

	return str;
}

bool ECS_ComponentRegisterType(
	ECS *ecs,
	const char *type,
	component_create_func cr_func,
	component_delete_func dl_func,
	size_t type_size)
{
	assert(ecs && type);

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
		cr_func, dl_func,
		type_size,
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
