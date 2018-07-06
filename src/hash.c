#include "hash.h"
#include "zlib.h"
#include "mempool.h"
#include "manager.h"

#include <math.h>

/*
	A fairly simple string hash function, that has the advantage of generating
	identical results across invocations - allowing the generated hashes to be
	used as identifiers, even in files saved to disk between program runs.
*/
hash_t hash_string(const char *string)
{
	return hash_bytes(string, strlen(string));
}

hash_t hash_bytes(const char *string, size_t len)
{
	return crc32(crc32(0L, Z_NULL, 0), (unsigned char *)string, len);
}

/*
	Buckets are stored as chunks in a memory pool. Each chunk has the format:

	struct chunk {
		void **prev;
		hash_t hash;
		char[sizeof(data)] val;
	}
*/

#define LOAD_MAX 0.8f

// Walk backwards until condition c is met.
#define WALK_BACKWARDS(ht, idx, c) \
	bucket_t *ent = (ht)->buckets[(idx)]; \
	while (!(c)) ent = ent->prev

typedef struct bucket_t bucket_t;
struct bucket_t {
	bucket_t *prev;
	hash_t hash;
	// this stores the hashtable's data. It is aligned to the 8-byte boundary
	char data[] __attribute__((aligned(8)));
};

struct hashtable_t {
	size_t size;
	size_t entry_size;
	size_t data_size;
	size_t count;
	mempool_t *storage;
	bucket_t **buckets;
};

static inline size_t get_bucket_idx(hashtable_t *ht, hash_t hash)
{
	return hash % ht->size;
}

// Get an entry in a hashtable bucket.
static inline bucket_t* get_entry(hashtable_t *ht, hash_t hash)
{
	const size_t idx = get_bucket_idx(ht, hash);
	bucket_t *bucket_ptr = ht->buckets[idx];
	while (bucket_ptr != NULL) {
		if (bucket_ptr->hash == hash) return bucket_ptr;
		bucket_ptr = bucket_ptr->prev;
	}

	return NULL;
}

/* -------------------------------------------------------------------------- */

hashtable_t* ht_alloc(size_t size, size_t val_size)
{
	hashtable_t *ht = malloc(sizeof(hashtable_t));
	if (!ht) return NULL;

	ht->count = 0;
	ht->size = size == 0 ? 16 : size;
	ht->data_size = val_size;
	ht->entry_size = sizeof(bucket_t) + val_size;

	// There will always be room for at least ht->size entries in the table.
	// As a consequence, LOAD_MAX must never be > 1.0.
	ht->buckets = calloc(size, sizeof(bucket_t *));
	ht->storage = mp_init(ht->size, ht->entry_size);

	if (!ht->buckets || !ht->storage) {
		ht_free(ht);
		return NULL;
	}

	memset(ht->buckets, 0, ht->size * sizeof(bucket_t *));

	return ht;
}

void ht_free(hashtable_t *ht)
{
	assert(ht);

	// We won't free the hashtable unless all of the entries have been deleted,
	// as this would potentially leave dangling pointers.
	if (ht->count != 0) return;

	// free the bucket array and the hashtable structure.
	if (ht->buckets) free(ht->buckets);
	if (ht->storage) mp_destroy(ht->storage);
	free(ht);
}

/*
	resize the bucket list and redistribute all items in the hash table
	according to their new indexes
*/
static bool resize(hashtable_t *ht) {
	// Resize the bucket list
	size_t newsize = ht->size * 2;
	bucket_t **ptr = realloc(ht->buckets, sizeof(bucket_t *) * newsize);
	if (ptr == NULL) return false;

	// zero the new bucket pointers
	memset(ptr + ht->size, 0, (newsize - ht->size) * sizeof(bucket_t *));

	ht->buckets = ptr;
	ht->size = newsize;

	// Size of the hashtable changed, so we need to rehash all the entries.
	for (size_t idx = 0; idx < ht->size; idx++) {
		// the entry before this one in the chain.
		bucket_t *next = NULL;
		// the current entry
		bucket_t *entry = ht->buckets[idx];
		while (entry != NULL) {
			size_t n_idx = get_bucket_idx(ht, entry->hash);
			// move the entry to its proper bucket.
			if (n_idx != idx) {
				// remove the entry from the current chain.
				if (next == NULL) ht->buckets[idx] = entry->prev;
				else next->prev = entry->prev;

				// insert it into the other chain.
				entry->prev = ht->buckets[n_idx];
				ht->buckets[n_idx] = entry;

				// reset with the new top of this bucket.
				entry = next ? next->prev : ht->buckets[idx];
			}
			else {
				// walk through the chain.
				next = entry;
				entry = entry->prev;
			}
		}
	}

	return true;
}

void* ht_insert(hashtable_t *ht, hash_t hash, void *data)
{
	assert(ht && ht->buckets && ht->storage);

	if (ht->count > 0 && (float)ht->count / ht->size > LOAD_MAX) {
		if (!resize(ht)) return NULL;
	}

	// Get the bucket.
	const size_t bucket = get_bucket_idx(ht, hash);
	// If we don't have this hash stored already...
	bucket_t *entry = get_entry(ht, hash);
	if (!entry) {
		bucket_t *ptr = mp_alloc(ht->storage);
		if (!ptr) return NULL;
		memset((void *)ptr, 0, ht->entry_size);
		ptr->prev = ht->buckets[bucket];
		entry = ht->buckets[bucket] = ptr;
		ht->count++;
	}

	entry->hash = hash;
	if (data)
		memcpy(entry->data, data, ht->data_size);
	else
		memset(entry->data, 0, ht->data_size);

	// Return the pointer to the new data.
	return entry->data;
}

void* ht_get(hashtable_t *ht, hash_t hash)
{
	assert(ht && ht->buckets && ht->storage);

	bucket_t *ht_ent = get_entry(ht, hash);
	return ht_ent ? ht_ent->data : NULL;
}

// Get the next hashtable entry after the current.
// Calling the function with hash = 0 gets the first entry,
hash_t ht_next(hashtable_t *ht, hash_t hash)
{
	assert(ht && ht->buckets && ht->storage);

	if (ht->count < 1) return 0;

	size_t idx = get_bucket_idx(ht, hash);
	if (ht->buckets[idx]) {
		// If we have a bucket and our start hash is not at the end of the bucket:
		if (ht->buckets[idx]->hash != hash) {
			bucket_t *entry = ht->buckets[idx];
			// Find the hash and return the one after it.
			while (entry->prev) {
				if (entry->prev->hash == hash) return entry->hash;
				entry = entry->prev;
			}

			// If the starting hash is not in the table, return the first in
			// the appropriate bucket.
			return entry->hash;
		}
		// If the starting index is the end of the bucket, find the next bucket.
		else idx++;
	}

	// If we don't have a bucket, walk the hashtable until we find one.
	// Then, get the first entry in the bucket.
	for (size_t n_idx = idx; n_idx < ht->size; n_idx++) {
		if (ht->buckets[n_idx]) {
			WALK_BACKWARDS(ht, n_idx, !ent->prev);
			return ent->hash;
		}
	}

	return 0;
}

void ht_delete(hashtable_t *ht, hash_t hash)
{
	assert(ht && ht->buckets && ht->storage);

	size_t idx = get_bucket_idx(ht, hash);
	bucket_t *entry = get_entry(ht, hash);
	if (!entry) return;

	if (ht->buckets[idx] == entry) {
		// Remove this entity from the chain.
		ht->buckets[idx] = entry->prev;
	}
	else {
		// Get the next entry in the chain.
		WALK_BACKWARDS(ht, idx, ent->prev == entry);
		// Remove this entry.
		ent->prev = entry->prev;
	}

	// Deallocate the block.
	mp_free(ht->storage, entry);
	ht->count--;
}
