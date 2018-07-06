// event.h

#ifndef ECS_EVENT_H
#define ECS_EVENT_H

#include "ecs.h"

typedef void EventData;

/*
    A structure representing an event sent to a system.
*/
struct Event {
    // The type of the event. The meaning of this value is implementation defined.
    hash_t id;
    // The id of the entity this event is targeting. Can be null.
    hash_t target;
    // A pointer to some userdata the event carries.
    EventData *data;
};

/*
    An Event Queue is a simple FIFO of Events.
*/
typedef struct {
    dynarray_t evs;
} EventQueue;

/*
    Allocate a new EventQueue.
*/
EventQueue* EventQueue_New();

/*
    Free a previously-allocated EventQueue.
*/
void EventQueue_Free(EventQueue *queue);

/*
    Return a pointer to an event in the queue. An index of 0 is the front
    of the queue.

    Returns NULL if there is no event at that index.
*/
Event* EventQueue_Peek(EventQueue *queue, int idx);

/*
    Remove an event from the front of the queue.
*/
bool EventQueue_Pop(EventQueue *queue, Event *out_event);
void EventQueue_Push(EventQueue *queue, Event *event);

#endif /* end of include guard: ECS_EVENT_H */
