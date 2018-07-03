#include "hash.h"
#include "zlib.h"
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
	return crc32(crc32(0L, Z_NULL, 0), string, len);
}

#define LOAD_MAX 0.8f
#define RESIZE_FACTOR 1.6f

/*
	Buckets are stored as dynamic arrays for simplicity and performance.

	Hashtable buckets are stored as the pseudo-structure:

		struct bucket {
			size_t count;
			struct {
				hash_t hash;
				sizeof(val) entry;
			} entries[];
		}

	Because they can store items of various type, void* is necessary.
*/

// Get the bucket index a hash belongs to in a hashtable.
static size_t get_bucket_idx(hashtable_t *ht, hash_t hash)
{
	return hash % ht->size;
}

// Get the number of items in a bucket
static size_t get_bucket_size(hashtable_t *ht, size_t bucket)
{
	assert(ht && ht->ptr);
	if (bucket >= ht->size || ht->ptr[bucket] == NULL)
		return 0;
	return *(size_t*)ht->ptr[bucket];
}

// Get an entry in a hashtable bucket.
static void* get_entry(hashtable_t *ht, size_t bucket, size_t idx)
{
	if (bucket >= ht->size) return NULL;
	void *bucket_ptr = ht->ptr[bucket];
	if (bucket_ptr == NULL || idx >= *(size_t*)bucket_ptr) return NULL;

	return bucket_ptr + sizeof(size_t) + ht->entry_size * idx;
}

// Gets the index of an entry in a bucket. Returns 1+end if not present.
static size_t get_entry_idx(hashtable_t *ht, size_t bucket, hash_t hash)
{
	const size_t count = get_bucket_size(ht, bucket);
	for (size_t idx = 0; idx < count; idx++) {
		void *entry = get_entry(ht, bucket, idx);
		if (entry != NULL && *(hash_t*)entry == hash) {
			return idx;
		}
	}
	return count;
}

/* -------------------------------------------------------------------------- */

hashtable_t* ht_alloc(size_t count, size_t val_size)
{
	hashtable_t *ht = malloc(sizeof(hashtable_t));
	if (!ht) return NULL;

	ht->size = count == 0 ? 16 : count;
	ht->entry_size = sizeof(hash_t) + val_size;
	ht->count = 0;

	// allocate and clear the arrays (ensures all bucket pointers start at NULL)
	ht->ptr = calloc(count, sizeof(void*));
	if (!ht->ptr) return NULL;
	memset(ht->ptr, 0, count * sizeof(void*));

	return ht;
}

void ht_free(hashtable_t *ht)
{
	assert(ht);

	// We won't free the hashtable unless all of the entries have been deleted,
	// as this would potentially leave dangling pointers.
	if (ht->count != 0) return;

	// free all the buckets
	for (size_t idx = 0; idx < ht->size; idx++) {
		if (ht->ptr[idx] != NULL) free(ht->ptr[idx]);
	}

	// free the bucket array and the hashtable structure.
	free(ht->ptr);
	free(ht);
}

// redestribute all items in the hash table according to their new indexes
static void rehash(hashtable_t *ht) {

}

void* ht_insert(hashtable_t *ht, hash_t hash, void *data)
{
	assert(ht && ht->ptr);

	if (ht->count > 0 && (float)ht->size / ht->count > LOAD_MAX) {
		// Resize the bucket list by approx 1.5
		size_t newsize = floor((float)ht->size * RESIZE_FACTOR);
		void **ptr = realloc(ht->ptr, sizeof(void*) * newsize);
		if (ptr == NULL) return NULL;

		// zero the new bucket pointers
		memset(ptr + sizeof(void*) * ht->size, 0, sizeof(void*) * (newsize - ht->size));

		ht->ptr = ptr;
		ht->size = newsize;

		// Size of the hashtable changed, so we need to rehash all the entries.
		// Yay.
		rehash(ht);
	}

	// Get the bucket.
	const size_t bucket = get_bucket_idx(ht, hash);
	// If the bucket is empty, alloc a new array
	if (ht->ptr[bucket] == NULL) {
		// Allocate space for one item.
		const size_t size = sizeof(size_t) + ht->entry_size;
		void *ptr = malloc(size);
		if (ptr == NULL) return NULL;

		memset(ptr, 0, size);
		*(size_t *)ptr = 1;

		ht->ptr[bucket] = ptr;
	}

	// Check for a free slot.
	size_t bucket_size = get_bucket_size(ht, bucket);
	size_t entry_idx = get_entry_idx(ht, bucket, hash);
	void *ht_ent = NULL;
	// The hash is not already in the bucket.
	if (entry_idx == bucket_size) {
		// Check for free slots in the bucket.
		for (size_t idx = 0; idx < bucket_size; idx++) {
			void *entry = get_entry(ht, bucket, idx);
			if (entry && *(hash_t *)entry == 0) {
				ht_ent = entry;
				entry_idx = idx;
				break;
			}
		}

		// No free slot in the bucket, so make one.
		if (!ht_ent) {
			const size_t size = sizeof(size_t) + ht->entry_size * (bucket_size + 1);
			void *ptr = realloc(ht->ptr[bucket], size);
			if (ptr == NULL) return NULL;

			// Clear the new memory;
			memset(ptr + size - ht->entry_size, 0, ht->entry_size);
			*(size_t *)ptr = bucket_size + 1;

			ht->ptr[bucket] = ptr;
			entry_idx = bucket_size++;
		}
	}

	// FINALLY, get the pointer.
	ht_ent = get_entry(ht, bucket, entry_idx);
	if(ht_ent == NULL) return NULL;

	*(hash_t *)ht_ent = hash;
	ht_ent += sizeof(hash_t);
	const size_t data_size = ht->entry_size - sizeof(hash_t);

	if (data)
		memcpy(ht_ent, data, data_size);
	else
		memset(ht_ent, 0, data_size);

	// Return the pointer to the new data.
	return ht_ent;
}

void* ht_get(hashtable_t *ht, hash_t hash)
{
	assert(ht && ht->ptr);

	size_t bucket = get_bucket_idx(ht, hash);
	size_t entry = get_entry_idx(ht, bucket, hash);
	void *ht_ent = get_entry(ht, bucket, entry);

	return ht_ent == NULL ? ht_ent : ht_ent + sizeof(hash_t);
}

// Get the next hashtable entry after the current.
hash_t ht_next(hashtable_t *ht, hash_t hash)
{
	assert(ht && ht->ptr);

	if (ht->count < 1) return 0;

	size_t start = get_bucket_idx(ht, hash);
	size_t idx = start;
	hash_t *entry_hash = NULL;
	if (get_bucket_size(ht, start) > get_entry_idx(ht, start, hash) + 1) {
		// check if there's entries in the bucket after the current one.
		entry_hash = get_entry(ht, start, get_entry_idx(ht, start, hash) + 1);
	}
	else {
		// loop through the hashtable until we find a non-null element
		// or get back to the start
		do {
			idx = idx++ % ht->size;
			if (idx == start) return 0;
		} while(get_bucket_size(ht, idx) == 0);

		entry_hash = get_entry(ht, idx, 0);
	}

	return entry_hash == NULL ? 0 : *entry_hash;
}

void ht_delete(hashtable_t *ht, hash_t hash)
{
	assert(ht && ht->ptr);

	const size_t bucket = get_bucket_idx(ht, hash);
	const size_t entry = get_entry_idx(ht, bucket, hash);

	// if it doesn't exist, don't need to delete it.
	if (get_bucket_size(ht, bucket) > entry) return;

	// clear the entry.
	void *entry_ptr = get_entry(ht, bucket, entry);
	memset(entry_ptr, 0, ht->entry_size);
}
