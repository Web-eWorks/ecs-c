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

struct mempool_t {
   void **free;
   void **last;
   void **seg;
   size_t entry_size;
   size_t min_size;
   // Align the contents of the pool to 8-byte boundaries for performance.
   char data[] __attribute__((aligned(8)));
};

#include <stdio.h>

// Run through a new segment and setup the next pointers.
static void init_segment(mempool_t *mp, void *segment, size_t size)
{
    void **ptr = segment;
    // setup the linked list
    for (size_t idx = 0; idx < size; idx++) {
        ptr = ptr[0] = (char *)ptr + mp->entry_size;
    }
    mp->last = segment + (size - 1) * mp->entry_size;
    *mp->last = NULL;
}

mempool_t* mp_init(size_t min_size, size_t entry_size)
{
    // Must have at least one element (though larger powers of two are
    // better for performance)
    min_size = min_size > 0 ? min_size : 1;

    // Must have at least the size of a void* to store the header.
    entry_size = entry_size > sizeof(void *) ? entry_size : sizeof(void *);
    // Align to 8 bytes
    if (entry_size % 8) entry_size += 8 - entry_size % 8;
    assert(entry_size % 8 == 0);

    mempool_t *mp = malloc(sizeof(mempool_t) + min_size * entry_size);
    if (!mp) return NULL;

    // setup all the variables
    mp->free = (void**)mp->data;
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
    while (pool->seg != NULL) {
        void **s = pool->seg;
        pool->seg = (void **)*s;
        free(s);
    }

    free(pool);
}

bool insert_segment(mempool_t *pool)
{
    void **seg = malloc(sizeof(void *) + pool->min_size * pool->entry_size);
    if (!seg) return false;

    // The segment points to the next in the chain,
    seg[0] = pool->seg;
    // and the pool points to the end of the chain
    pool->seg = seg;

    // The last entry in the old pool now points to the first entry in the new
    // pool.
    *pool->last = seg + 1;
    init_segment(pool, seg + 1, pool->min_size);

    return true;
}

void* mp_alloc(mempool_t *pool)
{
    assert(pool && pool->free);

    // If we're at the last element in the pool, create a new segment.
    if (pool->free == pool->last) {
        if (!insert_segment(pool)) return NULL;
    }

    // Advance the free pointer.
    void *n = pool->free;
    pool->free = *pool->free;

    return n;
}

void mp_free(mempool_t *pool, void *ptr)
{
    assert(pool);

    // trust the pointer resides in the pool, and append it to the start of the chain.
    *(void **)ptr = pool->free;
    pool->free = ptr;

    // Voila! That's it!
}
