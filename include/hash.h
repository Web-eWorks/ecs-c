// hash.h

#ifndef ECS_HASH_H
#define ECS_HASH_H

#include <stddef.h>

typedef unsigned int hash_t;

typedef struct {
	size_t size;
	size_t entry_size;
	size_t count;
	void **ptr;
} hashtable_t;

hash_t hash_string(const char *str);
hash_t hash_bytes(const char *bytes, const size_t len);

/*
	Allocate and delete a hashtable.
	The initial count of slots should be a power of two, and defaults to 16 if
	not specified.
*/
hashtable_t* ht_alloc(const size_t count, const size_t val_size);
void ht_free(hashtable_t* ht);

/*
	Create / insert data into the hashtable at a certain index.
	If data is NULL, zero-initializes the allocated memory.

	Returns a pointer to the allocated entry.
*/
void* ht_insert(hashtable_t *ht, hash_t hash, void *data);

/*
	Return the hashtable entry referred to by hash.
*/
void* ht_get(hashtable_t *ht, hash_t hash);

/*
	Get the next hashtable entry after the current key.
	Returns NULL if the table has no more items.
*/
hash_t ht_next(hashtable_t *ht, hash_t hash);

/*
	Remove an entry from the hashtable. Frees the entry's memory.
*/
void ht_delete(hashtable_t *ht, hash_t hash);

#define HT_FOR(HT, START) for (hash_t idx = START; (idx = ht_next(HT, idx)) != 0;)

#endif
