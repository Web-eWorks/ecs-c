#include "event.h"

#include <assert.h>

EventQueue* EventQueue_New()
{
    EventQueue *queue = malloc(sizeof(EventQueue));
    if (!queue) return NULL;

    if (!dyn_alloc(&queue->evs, 16, sizeof(Event))) return NULL;

    return queue;
}

void EventQueue_Free(EventQueue *queue)
{
    assert(queue && queue->evs.ptr);

    dyn_free(&queue->evs);
    free(queue);
}

Event* EventQueue_Peek(EventQueue *queue, int idx)
{
    assert(queue && queue->evs.ptr);

    return dyn_get(&queue->evs, idx);
}

bool EventQueue_Pop(EventQueue *queue, Event *ev)
{
    assert(queue && queue->evs.ptr);

    void *event = dyn_get(&queue->evs, 0);
    if (!ev) return false;

    if (ev) {
        *ev = *(Event *)event;
    }
    
    for (size_t idx = 1; idx < queue->evs.size; idx++) {
        dyn_swap(&queue->evs, idx-1, idx);
    }
    dyn_delete(&queue->evs, -1);

    return true;
}

void EventQueue_Push(EventQueue *queue, Event *event)
{
    assert(queue && queue->evs.ptr && event);

    dyn_insert(&queue->evs, queue->evs.size, event);
}
