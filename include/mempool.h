// mempool.h

#ifndef ECS_MEMPOOL_H
#define ECS_MEMPOOL_H

#include <stdlib.h>

typedef struct mempool_t mempool_t;

/*
    Allocate and return a new mempool_t.

    `min_size` sets the default allocation count (the number of chunks in the
    pool). When the pool grows, it will grow by this amount.

    `entry_size` sets the allocation size (the size of the chunks)

    The first chunk is always aligned to an 8-byte boundary for performance. It
    is the responsibility of the caller to select a chunk size that maintains
    the alignment requirements of the data stored therein.

    It is guaranteed that all chunks are effectively contiguous, so the chunk
    size must be a multiple of the alignment requirement of the stored data.
*/
mempool_t* mp_init(size_t min_size, size_t entry_size);

/*
    Destroy a mempool_t. This also frees all items allocated inside it.
*/
void mp_destroy(mempool_t *pool);

/*
    Allocate a new pointer in the pool.
*/
void* mp_alloc(mempool_t *pool);

/*
    Free a pointer from the pool.
    The behaviour of this function is undefined if the passed pointer was not
    allocated by the same pool.
*/
void mp_free(mempool_t *pool, void *ptr);

#endif /* end of include guard: ECS_MEMPOOL_H */
