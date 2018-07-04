#include "system.h"
#include "manager.h"

bool ECS_SystemRegister(
    ECS *ecs,
    const char *name,
    system_update_func update,
    system_event_func event,
    const char **collection,
    System *data)
{
    assert(ecs && name && update);

    SystemInfo info = {
        malloc(strlen(name) + 1),
        hash_string(name),
        update,
        event,
        {},
        data,
        EventQueue_New()
    };

    // Copy the name string.
    if (!info.name) return false;
    strcpy((char *)info.name, name);

    // Convert the collection to type ids for speed.
    if (collection) {
        int idx = 0;
        while (collection[idx]) idx++;

        info.collection.size = idx;
        info.collection.types = calloc(info.collection.size, sizeof(hash_t));
        if (!info.collection.types) {
            free((char *)info.name);
            return false;
        }

        for (int idx = 0; idx < info.collection.size; idx++) {
            info.collection.types[idx] = hash_string(collection[idx]);
        }
    }

    return Manager_RegisterSystem(ecs, &info);
}

SystemInfo* ECS_SystemGet(ECS *ecs, const char *name)
{
    assert(ecs && ecs->systems && name);
    return Manager_GetSystem(ecs, name);
}

void ECS_SystemUnregister(ECS *ecs, const char *name)
{
    assert(ecs && ecs->systems && name);
    Manager_UnregisterSystem(ecs, name);
}
