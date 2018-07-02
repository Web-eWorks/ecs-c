
#include "dynarray.h"

#include <assert.h>
#include <string.h>

static inline int clamp(int val, int min, int max)
{
	const int t = val < min ? min : val;
	return t > max ? max : t;
}

#define GET_RIDX(size, idx) (idx) < 0 ? clamp((size) + (idx), 0, (size)) : (idx)

bool dyn_alloc(dynarray_t *arr, size_t size, size_t entry_size)
{
	assert(arr);
	if (arr->ptr) dyn_free(arr);
	
	arr->size = 0;
	arr->entry_size = entry_size;
	arr->capacity = size;
	
	arr->ptr = calloc(size, entry_size);
	return arr->ptr != NULL;
}

void dyn_free(dynarray_t *arr)
{
	assert(arr);
	free(arr->ptr);
}

void* dyn_insert(dynarray_t *arr, int idx, void *data)
{
	assert(arr && arr->ptr);
	
	const size_t r_idx = GET_RIDX(arr->size, idx);
	if (r_idx >= arr->capacity) {
		if (!dyn_resize(arr, r_idx + 1)) return NULL;
	}
	
	void *ptr = arr->ptr + arr->entry_size * r_idx;
	
	if (data == NULL) memset(ptr, 0, arr->entry_size);
	else memcpy(ptr, data, arr->entry_size);
	
	if (r_idx >= arr->size) {
		arr->size = r_idx + 1;
	}
	
	return ptr;
}

void* dyn_get(dynarray_t *arr, int idx)
{
	assert(arr && arr->ptr);
	
	const size_t r_idx = GET_RIDX(arr->size, idx);
	if (r_idx >= arr->size) return NULL;
	
	return arr->ptr + r_idx * arr->entry_size;
}

void dyn_delete(dynarray_t *arr, int idx)
{
	assert(arr && arr->ptr);
	
	const size_t r_idx = GET_RIDX(arr->size, idx);
	if (r_idx >= arr->size) return;

	void *ptr = arr->ptr + r_idx * arr->entry_size;
	memset(ptr, 0, arr->entry_size);
	
	if (r_idx == arr->size - 1) --arr->size;
}

bool dyn_swap(dynarray_t *arr, int idx_a, int idx_b)
{
	assert(arr && arr->ptr);
	
	const size_t r_idx_a = GET_RIDX(arr->size, idx_a);
	const size_t r_idx_b = GET_RIDX(arr->size, idx_b);
	
	if (r_idx_a >= arr->size || r_idx_b >= arr->size) return false;
	if (r_idx_a == r_idx_b) return true;
	
	void *ptr = malloc(arr->entry_size);
	if (!ptr) return false;
	
	void *addr_a = arr->ptr + r_idx_a * arr->entry_size;
	void *addr_b = arr->ptr + r_idx_b * arr->entry_size;
	
	memmove(ptr, addr_a, arr->entry_size);
	memmove(addr_a, addr_b, arr->entry_size);
	memmove(addr_b, ptr, arr->entry_size);
	free(ptr);
	
	return true;
}

bool dyn_reserve(dynarray_t *arr, size_t newcap)
{
	assert(arr);
	if (arr->size + newcap <= arr->capacity) {
		return true;
	}
	
	return dyn_resize(arr, arr->size + newcap);
}

bool dyn_resize(dynarray_t *arr, size_t newcap)
{
	assert(arr);
	if (newcap <= arr->size) {
		return false;
	}

	void *ptr = realloc(arr->ptr, newcap * arr->entry_size);
	if (ptr == NULL) {
		return false;
	}
	
	arr->ptr = ptr;
	arr->capacity = newcap;
	
	return true;
}
