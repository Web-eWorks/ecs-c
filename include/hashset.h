// hashset.h - A hash-lookup set implementation

#ifndef ECS_HASHSET_H
#define ECS_HASHSET_H

#include "hash.h"
#include <stdbool.h>

/*
    A very simple implementation of a constant-lookup set, based on hashed
    strings. This is used to handle dependency resolution, etc.
*/
typedef struct hashset_t hashset_t;

hashset_t* hs_alloc(size_t init_size);
void hs_free(hashset_t *hs);

void hs_set(hashset_t *hs, hash_t hash);
bool hs_get(hashset_t *hs, hash_t hash);
hash_t hs_next(hashset_t *hs, hash_t hash);
void hs_clear(hashset_t *hs, hash_t hash);

#define HS_FOR(hs) \
    for (hash_t idx = 0; (idx = hs_next(hs, idx));)

#endif /* end of include guard: ECS_HASHSET_H */
