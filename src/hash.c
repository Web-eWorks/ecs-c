#include "hash.h"
#include "zlib.h"
#include "manager.h"

hash_t hash_string(const char *string)
{
	return hash_bytes(string, strlen(string));
}

hash_t hash_bytes(const char *string, size_t len)
{
	return crc32(crc32(0L, Z_NULL, 0), string, len);
}

/*
	Buckets are stored as dynamic arrays for simplicity and performance. The
	hash table's underlying bucket storage is resized with a factor of 1.5.

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
	void *bucket_ptr = ht->ptr[idx];
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

// Allocate a new hashtable
hashtable_t* ht_alloc(size_t count, size_t val_size)
{
	hashtable_t *ht = malloc(sizeof(hashtable_t));
	if (!ht) return NULL;

	ht->size = count == 0 ? 16 : count;
	ht->entry_size = sizeof(hash_t) + val_size;
	ht->count = 0;
	ht->ptr = calloc(count, sizeof(void*));
	memset(ht->ptr, 0, count * sizeof(void*));
	
	return ht;
}

// Free an old hashtable
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

	size_t bucket = get_bucket_idx(ht, hash);

	// TODO insert into table
	void *ht_ent;
	
	if (data)
		memcpy(ht_ent, data, ht->entry_size);
	else
		memset(ht_ent, 0, ht->entry_size);
	
	return ht_ent;
}

void* ht_get(hashtable_t *ht, hash_t hash)
{
	assert(ht && ht->ptr);
	
	// TODO: table lookup;
	void *ht_ent;
	
	return ht_ent;
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
}
