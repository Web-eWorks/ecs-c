#include "mempool.h"
#include <assert.h>
#include <stdbool.h>

/*
    Data is stored in segments of memory, each min_size * entry_size bytes
    long.

    The mempool is comprised of one static segment containing the mempool_t
    structure and the first chunk of data, and any amount of dynamic segments,
    each containing a header void** that points to the previous dynamic
    segment.

    The mempool keeps three dynamic pointers for management:

    pool->free
        Points to the first available pool slot.
    pool->last
        Points to the last available pool slot.
        When pool->free == pool->last, there is one slot remaining, and the
        pool needs to allocate a new segment.
    pool->seg
        A pointer to the last dynamic segment in the chain.

*/

// Run through a new segment and setup the next pointers.
static void init_segment(mempool_t *mp, void *segment, size_t size)
{
    // setup the linked list
    for (size_t idx = 0; idx < size; idx++) {
        void **ptr = segment + mp->entry_size * idx;
        *ptr = ptr + mp->entry_size;
    }
    mp->last = segment + (size - 1) * mp->entry_size;
    *mp->last = NULL;
}

mempool_t* mp_init(size_t min_size, size_t entry_size)
{
    min_size = min_size > 0 ? min_size : 1;
    entry_size = entry_size > sizeof(void *) ? entry_size : sizeof(void *);

    mempool_t *mp = malloc(sizeof(mempool_t) + min_size * entry_size);
    if (!mp) return NULL;

    // setup all the variables
    mp->free = (void *)&mp[1];
    mp->entry_size = entry_size;
    mp->min_size = min_size;
    mp->seg = NULL;

    init_segment(mp, mp->free, min_size);
    return mp;
}

void mp_destroy(mempool_t *pool)
{
    assert(pool);

    // destroy each segment.
    void **s = pool->seg;
    while (pool->seg != NULL) {
        pool->seg = (void **)*s;
        free(s);
    }

    free(pool);
}

bool insert_segment(mempool_t *pool)
{
    void **seg = malloc(sizeof(void *) + pool->min_size * pool->entry_size);
    if (!seg) return false;

    *seg = pool->seg;
    pool->seg = seg;
    init_segment(pool, seg + 1, pool->min_size);

    return true;
}

void* mp_alloc(mempool_t *pool)
{
    assert(pool);

    // If we're at the last element in the pool, create a new segment.
    if (pool->free == pool->last && !insert_segment(pool)) return NULL;

    // Advance the free pointer.
    void *n = pool->free;
    pool->free = *pool->free;

    return n;
}

void mp_free(mempool_t *pool, void *ptr)
{
    assert(pool);

    // trust the pointer resides in the pool.
    *(void **)ptr = pool->free;
    pool->free = ptr;

    // Voila! That's it!
}
