// hashset.c - A hash-based set implementation.

#include "hashset.h"
#include "mempool.h"
#include "macros.h"

#include <stdlib.h>

#define GET_IDX(hs, hash) ((hash) % (hs)->size)
#define GET_BUCKET(hs, hash) hs->buckets[GET_IDX(hs, hash)]

typedef struct bucket_t bucket_t;

struct hashset_t {
    hash_t size;
    hash_t count;
    bucket_t **buckets;
    mempool_t *storage;
};

struct bucket_t {
    hash_t hash;
    bucket_t *next;
};

hashset_t* hs_alloc(size_t initial_size)
{
    hashset_t *hs = malloc(sizeof(hashset_t));
    hs->buckets = calloc(initial_size, sizeof(bucket_t *));
    hs->storage = mp_init(initial_size, sizeof(bucket_t));
    hs->size = initial_size;

    return hs;
}

void hs_delete(hashset_t *hs)
{
    mp_destroy(hs->storage);
    free(hs->buckets);
    free(hs);
}

void hs_resize(hashset_t *hs)
{
    size_t oldsize = hs->size;
    size_t newsize = hs->size * 2;
    void *ptr = realloc(hs->buckets, sizeof(bucket_t *) * newsize);
    ERR_RET(ptr, "Error allocating memory for hashset.\n");

    hs->size = newsize;
    hs->buckets = ptr;

    for (size_t idx = 0; idx < oldsize; idx++) {
        bucket_t *bk = hs->buckets[idx];
        bucket_t **last = &hs->buckets[idx];

        if (!bk) continue;

        while(bk) {
            size_t n_idx = GET_IDX(hs, bk->hash);
            if (n_idx == idx) continue;

            // Shuffle around all of our pointers to insert the bucket in the
            // list at the new position.
            bucket_t *next = bk->next;
            bucket_t *bk2 = hs->buckets[n_idx];
            hs->buckets[n_idx] = bk;
            bk->next = bk2;

            bk = next;
            (*last)->next = bk;
        }
    }
}

void hs_set(hashset_t *hs, hash_t hash)
{
    bucket_t *bk = GET_BUCKET(hs, hash);
    while (bk) {
        if (bk->hash == hash) return;
        if (!bk->next) break;
        bk = bk->next;
    }

    if (hs->count > (float)hs->size * 0.7) hs_resize(hs);

    bucket_t *next = mp_alloc(hs->storage);
    next->hash = hash;
    bk->next = next;
    hs->count++;
}

bool hs_get(hashset_t *hs, hash_t hash)
{
    bucket_t *bk = GET_BUCKET(hs, hash);
    // Loop through until we encounter NULL or the hash.
    while (bk && bk->hash != hash) bk = bk->next;

    return bk != NULL;
}

hash_t hs_next(hashset_t *hs, hash_t hash)
{
    if (!hs->count) return 0;

    size_t idx = GET_IDX(hs, hash);
    bucket_t *bk = hs->buckets[idx];
    
    // Walk the current bucket until we find the start hash or the end
    while (bk) {
        if (bk->next && bk->next->hash == hash) return bk->hash;
        bk = bk->next;
	}

    // Find the next filled bucket.
    while (idx < hs->size && !bk) bk = hs->buckets[++idx];
    if (!bk) return 0;

    // Get the last entry in the bucket.
    while(bk->next) bk = bk->next;
    return bk->hash;
}

void hs_clear(hashset_t *hs, hash_t hash)
{
    bucket_t *bk = GET_BUCKET(hs, hash);
    bucket_t **bk2 = &GET_BUCKET(hs, hash);

    // Remove the hash from the bucket and close the gaps.
    while (bk) {
        if (bk->hash == hash) {
            *bk2 = bk->next;
            mp_free(hs->storage, bk);
            hs->count--;
            return;
        }

        if (!bk->next) return;
        bk2 = &bk->next;
        bk = bk->next;
    }
}
