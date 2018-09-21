#include "system.h"
#include "manager.h"

bool ECS_SystemRegister(ECS *ecs, const SystemRegistryInfo *reg, void *data)
{
    assert(ecs && reg->name && reg->update);

    const char *name = reg->name;
    const SystemUpdateInfo *update_info = reg->update_info;

    System info;
    info.name = malloc(strlen(name) + 1);
    info.name_hash = hash_string(name);

    // Copy the name string.
    if (!info.name) return false;
    strcpy((char *)info.name, name);

    info.udata = data;

    info.up_func = reg->update;
    info.ev_func = reg->event;

    info.archetype = reg->archetype;

    info.is_thread_safe = update_info->IsThreadSafe
        && !update_info->UpdatesOtherEntities
        && !update_info->CreatesOrDeletesEntities;

    // If we have dependencies, create a hash set to store them in.
    if (update_info->AfterSystems) {
        info.dependencies = hs_alloc(16);
        for (size_t idx = 0; update_info->AfterSystems[idx] != NULL; idx++) {
            hs_set(info.dependencies, hash_string(update_info->AfterSystems[idx]));
        }
    }

    info.ev_queue = EventQueue_New();
    info.ent_queue = NULL;

    return Manager_RegisterSystem(ecs, &info);
}

System* ECS_SystemGet(ECS *ecs, const char *name)
{
    assert(ecs && ecs->systems && name);
    return Manager_GetSystem(ecs, name);
}

void ECS_SystemUnregister(ECS *ecs, const char *name)
{
    assert(ecs && ecs->systems && name);

    System *info = ht_get(ecs->systems, hash_string(name));
    if (!info) return;
    Manager_UnregisterSystem(ecs, info);
}
