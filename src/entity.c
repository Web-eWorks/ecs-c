#include "entity.h"
#include "manager.h"

Entity ECS_EntityNew(ECS *ecs, EntityArchetype *archetype)
{
	assert(ecs);

	Entity entity = Manager_CreateEntity(ecs);
	if (archetype) {
		for (uint32_t idx = 0; idx < archetype->size; idx++) {
			hash_t id = archetype->components[idx];

			ComponentType *cm_type = ht_get(ecs->cm_types, id);
			ERR_CONTINUE(cm_type, "Error creating component: unregistered type %08x", id);

			Component *comp = Manager_CreateComponent(ecs, cm_type, entity);
			ERR_NO_RET(comp, "Error creating component of type %s.\n", cm_type->type);
		}

		Manager_UpdateCollections(ecs, entity);
	}

	return entity;
}

void ECS_EntityDelete(ECS *ecs, Entity entity)
{
	assert(ecs);

	Manager_DeleteEntity(ecs, entity);
}

bool ECS_EntityExists(ECS *ecs, Entity entity)
{
	assert(ecs);

	return ha_get(ecs->entities, entity) != NULL;
}

const char* ECS_EntityToString(Entity entity)
{
	char *str = malloc(24);
	snprintf(str, 24, "Entity (%08x)", entity);

	return str;
}

#define GET_TYPE(ecs, type, ret) ComponentType *cm_type = ht_get(ecs->cm_types, type); \
	if (!cm_type) { \
		ECS_ERROR(ecs, "Unknown component type %x.", type); \
		return ret; \
	}

Component* ECS_EntityAddComponent(ECS *ecs, Entity entity, hash_t type)
{
	assert(ecs);

	GET_TYPE(ecs, type, NULL);

	// If we already have a component on the entity, return it.
	Component *comp = ht_get(cm_type->components, entity);
	if (comp) return comp;

	// Otherwise, create the new component.
	comp = Manager_CreateComponent(ecs, cm_type, entity);

	// Update systems' entity queues.
	// We do this at the AddComponent / RemoveComponent step to gain performance.
	// Adding components to entities takes about 0.5x more time, but it gains an
	// immense amount of performance on the update step.
	Manager_UpdateCollections(ecs, entity);

	return comp;
}

Component* ECS_EntityGetComponent(ECS *ecs, Entity entity, hash_t type)
{
	assert(ecs);

	GET_TYPE(ecs, type, NULL);
	return ht_get(cm_type->components, entity);
}

void ECS_EntityRemoveComponent(ECS *ecs, Entity entity, hash_t type)
{
	assert(ecs);

	GET_TYPE(ecs, type,);

	if (!ht_get(cm_type->components, entity)) return;

	Manager_DeleteComponent(ecs, cm_type, entity);
	Manager_UpdateCollections(ecs, entity);
}

EntityArchetype* ECS_EntityRegisterArchetype(ECS *ecs, const char *name, const char **components)
{
	assert(ecs && name && components);

	EntityArchetype *arch = malloc(sizeof(EntityArchetype));
	ERR_OOM(arch, "creating EntityArchetype");

	arch->name = malloc(strlen(name) + 1);
	ERR_OOM(arch->name, "creating EntityArchetype");
	strcpy((char *)arch->name, name);

	arch->name_hash = hash_string(name);

	arch->size = string_arr_to_type(&arch->components, components);
	ERR_OOM(arch->components, "creating EntityArchetype");

	for (size_t idx = 0; idx < arch->size; idx++) {
		hash_t id = arch->components[idx];
		ComponentType *cm_type = ht_get(ecs->cm_types, id);
		ERR_RET_NULL(cm_type, "Error creating EntityArchetype: unknown component type %08x", id);
	}

	return arch;
}
