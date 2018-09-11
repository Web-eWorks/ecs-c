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
    // Whether the Event code should free the event's data.
    bool should_free;
};

/*
    An Event Queue is a simple FIFO of Events.
*/
typedef dynarray_t EventQueue;

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
    Remove an event from the front of the queue. If out_event is not NULL,
    it is the caller's responsibility to handle freeing the data ptr.
*/
bool EventQueue_Pop(EventQueue *queue, Event *out_event);

/*
    Add an event to the queue.
*/
void EventQueue_Push(EventQueue *queue, Event *event);

/*
    Remove all events from the queue.
*/
void EventQueue_Clear(EventQueue *queue);

#endif /* end of include guard: ECS_EVENT_H */
