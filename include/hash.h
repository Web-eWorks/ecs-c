// hash.h

#ifndef ECS_HASH_H
#define ECS_HASH_H

#include <stddef.h>
#include "mempool.h"

/*
    The hash type. By default, hashes are stored as 32-bit unsigned integers,
    with a maximum value of ~4 billion entries per hash table.

    If this limit is required to be breached, compile with ECS_BIG_HASH defined
    to get 64-bit hashes. This should never be required.
*/
#ifdef ECS_BIG_HASH
typedef unsigned long int hash_t;
#else
typedef unsigned int hash_t;
#endif

/*
    A hashtable implementation.

    Due to the use of an unsigned integer for the hash type, the index '0'
    does double-duty as both a regular index and an invalid index.

    The functions ht_next, ht_get_free, and ht_get_next_free treat 0 as an
    invalid or un-indexable value. All other functions treat 0 as valid.
*/
typedef struct hashtable_t hashtable_t;

hash_t hash_string(const char *str);
hash_t hash_bytes(const char *bytes, const size_t len);

/*
	Allocate and delete a hashtable.
	The initial count of slots should be a power of two, and defaults to 16 if
	not specified.

    Deletion of a hashtable is not thread safe.
*/
hashtable_t* ht_alloc(const size_t count, const size_t val_size);
void ht_free(hashtable_t* ht);

/*
	Create / insert data into the hashtable at a certain index.
	If data is NULL, zero-initializes the allocated memory.

	Returns a pointer to the allocated entry.

    This function is not thread safe.
*/
void* ht_insert(hashtable_t *ht, hash_t hash, void *data);

/*
	Return the hashtable entry referred to by hash.

    This function does not modify the table and is thread safe.
*/
void* ht_get(hashtable_t *ht, hash_t hash);

/*
    Get the number of elements in the hash table.

    This function does not modify the table and is thread-safe.
*/
size_t ht_len(hashtable_t *ht);

/*
	Get the next hashtable entry after the current key.
	Returns NULL if the table has no more items.

    This function has a best-case complexity of O(1) with a tightly packed
    table, and a worst-case complexity of O(N).

    This function does not modify the table and is thread-safe.
*/
hash_t ht_next(hashtable_t *ht, hash_t hash);

/*
    Returns the first available free index in the hashtable.
    Returns NULL if there are no free indexes.

    This function has a worst-case complexity of O(N), but is usually O(1).

    This function does not modify the table and is thread-safe.
*/
hash_t ht_get_free(hashtable_t *ht);

/*
    Returns the next free hash index after the given one. Otherwise, this
    function behaves like ht_get_free().
*/
hash_t ht_next_free(hashtable_t *ht, hash_t hash);

/*
	Remove an entry from the hashtable. Frees the entry's memory.

    This function is not thread safe.
*/
void ht_delete(hashtable_t *ht, hash_t hash);

#define HT_FOR(HT, START) for (hash_t idx = START; (idx = ht_next(HT, idx)) != 0;)
#define HT_RANGE_FOR(HT, S, E) for (hash_t idx = S; (idx = ht_next(HT, idx)) != 0 && idx != E;)

#endif
