#include "system.h"
#include "manager.h"

static int string_arr_to_type(hash_t **dst, const char **src)
{
    if (!dst || !src) return -1;

    size_t idx = 0;
    while(src[idx]) idx++;

    hash_t *ptr = calloc(idx, sizeof(hash_t));
    if (!ptr) return -1;

    dst[0] = ptr;
    for (size_t _i = 0; _i < idx; _i++) {
        ptr[_i] = hash_string(src[_i]);
    }

    return idx;
}

bool System_CreateCollection(SystemCollection *coll, const char **collection)
{
    coll->size = string_arr_to_type(&coll->types, collection);;
    if (!coll->types) return false;
    coll->comps = calloc(coll->size, sizeof(Component*));
    if (!coll->comps) return false;

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
    SystemUpdateInfo update_info,
    void *data)
{
    assert(ecs && name && update);

    System info;
    info.name = malloc(strlen(name) + 1);
    info.name_hash = hash_string(name);

    // Copy the name string.
    if (!info.name) return false;
    strcpy((char *)info.name, name);

    info.udata = data;

    info.up_func = update;
    info.ev_func = event;

    SystemCollection coll = { 0 };
    info.collection = coll;

    info.is_thread_safe = update_info.IsThreadSafe;
    info.updates_other_entities = update_info.UpdatesOtherEntities;
    if (info.updates_other_entities) info.is_thread_safe = false;

    info.before_systems = info.after_systems = NULL;
    if (update_info.BeforeSystems) string_arr_to_type(&info.before_systems, update_info.BeforeSystems);
    if (update_info.AfterSystems) string_arr_to_type(&info.after_systems, update_info.AfterSystems);

    info.ev_queue = EventQueue_New();
    info.ent_queue = NULL;

    // Convert the collection to type ids for speed.
    if (update_info.Collection) {
        if (!System_CreateCollection(&info.collection, update_info.Collection)) {
            free((char *)info.name);
            System_DeleteCollection(&info.collection);
            return false;
        }
    }

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
