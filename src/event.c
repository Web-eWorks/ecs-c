#include "event.h"

#include <assert.h>

EventQueue* EventQueue_New()
{
    EventQueue *queue = malloc(sizeof(EventQueue));
    if (!queue) return NULL;

    if (!dyn_alloc(queue, 16, sizeof(Event))) return NULL;

    return queue;
}

void EventQueue_Free(EventQueue *queue)
{
    assert(queue && queue->ptr);

    dyn_free(queue);
    free(queue);
}

Event* EventQueue_Peek(EventQueue *queue, int idx)
{
    assert(queue && queue->ptr);

    return dyn_get(queue, idx);
}

bool EventQueue_Pop(EventQueue *queue, Event *ev)
{
    assert(queue && queue->ptr);

    Event *event = dyn_get(queue, 0);
    if (!ev) return false;

    // If we're copying the event somewhere, we don't want to free the data.
    if (ev) {
        *ev = *event;
    }
    // If we're not, the event is being deleted, and the data should follow.
    else if (ev->should_free) {
        free(event->data);
    }

    dyn_remove(queue, 0, false);
    return true;
}

void EventQueue_Push(EventQueue *queue, Event *event)
{
    assert(queue && queue->ptr && event);

    dyn_insert(queue, queue->size, event);
}

void EventQueue_Clear(EventQueue *queue)
{
    assert(queue && queue->ptr);

    DYN_FOR(*queue, 0) {
        Event *ev = dyn_get(queue, idx);
        if (ev->should_free) free(ev->data);
        dyn_delete(queue, idx);
    }

    queue->size = 0;
}
