// system.h

#ifndef ECS_SYSTEM_H
#define ECS_SYSTEM_H

#include "ecs.h"

/*
    This function is called every update cycle to allow the system to update
    components.

    It is passed an array of Component pointers, according to the collection
    generated by the system's collection function.
*/
typedef void (*system_update_func)(Component **comps, void *udata);

/*
    This function is called once to generate the collection of component types
    the system is interested in.

    The function should return an array of component type names, with the size
    of the array stored in the `size` variable.
*/
typedef const char** (*system_collection_func)(int *size, void *udata);

/*
    This function is called when an event is sent to the system.

    The function should return true if the event has been processed.
*/
typedef bool (*system_event_func)(Event *ev, void *udata);

/*
    Register a system.

    @param name: the name of the system.

    @param update: the update function to call when generating ECS updates.

    @param event: the event function to call when an event is generated for
    this system.

    @param collection: a NULL-delimited list of component names the system
    wants to act upon. Updates are generated for each entity that has all
    of these components.
*/
bool ECS_SystemRegister(
    ECS *ecs,
    const char *name,
    system_update_func update,
    system_event_func event,
    const char **collection,
    System *data
);

/*
    Unregister a system from the ECS. The calling code should free the system's
    userdata pointer if present.
*/
void ECS_SystemUnregister(ECS *ecs, const char *name);

/* -------------------------------------------------------------------------- */

/*
    Send an event to a system.
*/
bool ECS_SystemQueueEvent(ECS *ecs, const char *name);

/* -------------------------------------------------------------------------- */

/*
    Macros for easy creation of systems.

    SYSTEM() and SYSTEM_IMPL() declare a custom data struct for your system,
    which is registered with REGISTER_SYSTEM().

    REGISTER_SYSTEM_NO_UDATA() registers a system without any extra data.
*/
#define SYSTEM(T) \
    typedef struct T T;

#define SYSTEM_IMPL(T) \
    static inline void T##_update(Component **c, T *p); \
    static void T##_uf(Component **c, System *p) { return T##_update(c, (T *)p); }; \
    static inline bool T##_event(Event *e, T *p); \
    static bool T##_ef(Event *e, System *p) { return T##_event(e, (T *)p); };

#define REGISTER_SYSTEM(ECS, T, INST) \
    ECS_SystemRegister(ECS, #T, T##_uf, T##_ef, T##_collection, INST)

#define REGISTER_SYSTEM_NO_UDATA(ECS, T) \
    ECS_SystemRegister(ECS, #T, T##_update, T##_event, T##_collection, NULL)

#endif /* end of include guard: ECS_SYSTEM_H */
