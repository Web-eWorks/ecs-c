#include "system.h"
#include "manager.h"

bool System_CreateCollection(SystemCollection *coll, const char **collection)
{
    size_t idx = 0;
    while (collection[idx]) idx++;

    coll->size = idx;
    coll->types = calloc(idx, sizeof(hash_t));
    coll->comps = calloc(idx, sizeof(Component*));
    if (!coll->types || !coll->comps) {
        return false;
    }

    for (idx = 0; idx < coll->size; idx++) {
        coll->types[idx] = hash_string(collection[idx]);
    }

    return true;
}

void System_DeleteCollection(SystemCollection *coll)
{
    free(coll->types);
    free(coll->comps);
}

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
        { 0 },
        data,
        EventQueue_New()
    };

    // Copy the name string.
    if (!info.name) return false;
    strcpy((char *)info.name, name);

    // Convert the collection to type ids for speed.
    if (collection) {
        if (!System_CreateCollection(&info.collection, collection)) {
            free((char *)info.name);
            System_DeleteCollection(&info.collection);
            return false;
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

    SystemInfo *info = ht_get(ecs->systems, hash_string(name));
    if (!info) return;
    Manager_UnregisterSystem(ecs, info);
}
