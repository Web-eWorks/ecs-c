// event.h

#ifndef ECS_EVENT_H
#define ECS_EVENT_H

#include "ecs.h"

typedef void EventData;

struct Event {
    hash_t id;
    EventData *data;
};

typedef struct {
    dynarray_t evs;
} EventQueue;

EventQueue* EventQueue_New();
void EventQueue_Free(EventQueue *queue);

Event* EventQueue_Peek(EventQueue *queue, int idx);
bool EventQueue_Pop(EventQueue *queue, Event *out_event);
void EventQueue_Push(EventQueue *queue, Event *event);

#endif /* end of include guard: ECS_EVENT_H */
