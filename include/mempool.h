// mempool.h

#ifndef ECS_MEMPOOL_H
#define ECS_MEMPOOL_H

#include <stdlib.h>

typedef struct {
    void **free;
    void **last;
    void **seg;
    size_t entry_size;
    size_t min_size;
} mempool_t;

/*
    Allocate and return a new mempool_t.
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
