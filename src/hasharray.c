// hash_array.c

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "hasharray.h"
#include "mempool.h"

struct hasharray_t {
    size_t count;
    size_t capacity;
    size_t entry_size;
    hash_t first_free;
    hash_t last_filled;
    mempool_t *storage;
    void **entries;
};

hasharray_t* ha_alloc(size_t min_size, size_t entry_size)
{
    hasharray_t *ha = malloc(sizeof(hasharray_t));
    if (!ha) return NULL;

    ha->count = 0;
    ha->capacity = min_size;
    ha->entry_size = entry_size;
    ha->first_free = 0;
    ha->last_filled = 0;
    ha->storage = mp_init(min_size, entry_size);
    ha->entries = calloc(min_size, sizeof(void *));

    if (!ha->entries || !ha->storage) {
        ha_free(ha);
        return NULL;
    }

    return ha;
}

void ha_free(hasharray_t *ha)
{
    assert(ha);

    free(ha->entries);
    mp_destroy(ha->storage);
    free(ha);
}

void* ha_insert(hasharray_t *ha, hash_t idx, void *data) {
    assert(ha && ha->entries && ha->storage);

    // Resize the array to at least fit the new index.
    if (idx >= ha->capacity) {
        size_t newsize = sizeof(void *) * (idx + 1);
        size_t size_diff = newsize - (ha->capacity * sizeof(void *));
        void **ptr = realloc(ha->entries, newsize);
        if (!ptr) return NULL;

        memset(&ptr[ha->capacity], 0, size_diff);
        ha->capacity = idx + 1;
        ha->entries = ptr;
    }

    // Get space for the entry
    void *entry = ha->entries[idx];
    if (!entry) {
        entry = mp_alloc(ha->storage);
        if (!entry) return NULL;
        ha->entries[idx] = entry;
    }

    ha->count++;
    if (idx > ha->last_filled) {
        ha->last_filled = idx + 1;
    }
    if (idx == ha->first_free) {
        ha->first_free = ha_next_free(ha, ha->first_free);
    }

    if (data)
        memcpy(entry, data, ha->entry_size);
    else
        memset(entry, 0, ha->entry_size);

    return entry;
}

void* ha_insert_free(hasharray_t *ha, hash_t *idx, void *data)
{
    assert(ha && ha->entries && ha->storage);

    hash_t _idx = ha_first_free(ha);
    if (idx) *idx = _idx;

    return ha_insert(ha, _idx, data);
}

size_t ha_len(hasharray_t *ha)
{
    return ha->count;
}

void* ha_get(hasharray_t *ha, hash_t idx)
{
    if (idx < ha->capacity) return ha->entries[idx];
    return NULL;
}

hash_t ha_next(hasharray_t *ha, hash_t idx)
{
    assert(ha && ha->entries);

    // Cycle through until we find the next full entry or the last filled entry
    // in the array.
    while(++idx < ha->last_filled && !ha->entries[idx]) continue;
    return idx;
}

hash_t ha_first_free(hasharray_t *ha)
{
    assert(ha && ha->entries);

    hash_t idx = ha->first_free;
    if (idx >= ha->capacity) return ha->first_free;
    else if ( ha->entries[idx]) {
        idx = ha->first_free = ha_next_free(ha, idx);
    }

    return idx;
}

hash_t ha_next_free(hasharray_t *ha, hash_t idx)
{
    assert(ha && ha->entries);

    // Cycle through until we find the next free entry or the end of the array.
    // TODO: make this less O(N)? Maybe a balanced search of some sort?
    while(ha->capacity > ++idx && ha->entries[idx]) continue;
    return idx;
}

hash_t ha_last(hasharray_t *ha)
{
    return ha->last_filled;
}

void ha_delete(hasharray_t *ha, hash_t idx)
{
    assert(ha && ha->entries && ha->storage);

    if (!ha->entries[idx]) return;

    mp_free(ha->storage, ha->entries[idx]);
    ha->entries[idx] = NULL;
    ha->count--;

    if (idx < ha->first_free) ha->first_free = idx;
    if (idx == ha->last_filled) ha->last_filled--;
}
