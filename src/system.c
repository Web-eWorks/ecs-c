#include "system.h"
#include "manager.h"

bool ECS_SystemRegister(
    ECS *ecs,
    const char *name,
    system_update_func update,
    system_event_func event,
    system_collection_func collection,
    void *udata)
{

}

SystemInfo* ECS_SystemGet(ECS *ecs, const char *name)
{

}

void ECS_SystemUnregister(ECS *ecs, const char *name)
{

}
