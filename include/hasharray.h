// hash_array.h

#ifndef ECS_HASH_ARRAY_H
#define ECS_HASH_ARRAY_H

#include "hash.h"

/*
    The hash array is a dynamic array backed by the same implementation as a
    hash table. It has an index time of O(1) and maintains the location of
    all array elements, so pointers into the array are not affected by
    resizing.

    Additionally, due to the underlying data storage, consecutive elements are
    usually consecutive in memory, so cache coherency is preserved. This can
    be affected by out-of-order insertions and deletions however.
*/
typedef struct hasharray_t hasharray_t;

hasharray_t* ha_alloc(size_t num_entries, size_t entry_size);
void ha_free(hasharray_t *ha);

/*
    Insert an element into the array at index idx, optionally pre-filling it
    with the contents of data.
*/
void* ha_insert(hasharray_t *ha, hash_t idx, void *data);

/*
    Insert an element into the array at the first free index, optionally
    pre-filling it.

    Returns the index of the inserted item at idx.
*/
void* ha_insert_free(hasharray_t *ha, hash_t *idx, void *data);

/*
    Returns the number of entries in the table.
*/
size_t ha_len(hasharray_t *ha);

/*
    Get a pointer to the element at idx. Returns NULL if there is no element
    present.
*/
void* ha_get(hasharray_t *ha, hash_t idx);

/*
    Get the next element present in the array.
*/
hash_t ha_next(hasharray_t *ha, hash_t idx);

/*
    Get the first free slot in the array. To get the first full slot, use
    ha_next(ha, 0).
*/
hash_t ha_first_free(hasharray_t *ha);

/*
    Get the next free slot in the array after a specific slot.
*/
hash_t ha_next_free(hasharray_t *ha, hash_t idx);

/*
    Delete an element in the array.
*/
void ha_delete(hasharray_t *ha, hash_t idx);

/*
    Iterate through a hash array, starting at a specific index.
*/
#define HA_FOR(ha, val, start) \
    for (hash_t idx = start; (val = ha_get(ha, idx)) != NULL; idx = ha_next(ha, idx))

/*
    Iterate over a range in the hash array.
*/
#define HA_RANGE_FOR(ha, val, start, end) \
    for (hash_t idx = start; idx < end && (val = ha_get(ha, idx)) != NULL; idx = ha_next(ha, idx))


#endif /* end of include guard: ECS_HASH_ARRAY_H */
