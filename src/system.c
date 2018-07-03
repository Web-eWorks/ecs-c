#include "system.h"
#include "manager.h"

bool ECS_SystemRegister(
    ECS *ecs,
    const char *name,
    system_update_func update,
    system_collection_func collection,
    system_event_func event,
    void *udata)
{
    assert(ecs && name && update && collection);

    SystemInfo info = {
        malloc(strlen(name) + 1),
        hash_string(name),
        update,
        collection,
        event,
        udata,
        EventQueue_New()
    };

    if (!info.name) return false;
    strcpy((char *)info.name, name);

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
