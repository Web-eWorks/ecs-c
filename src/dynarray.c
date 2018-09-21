
#include "dynarray.h"
#include "macros.h"

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
	ERR_RET_NULL(dyn_resize(arr, (r_idx < arr->size ? arr->size : r_idx) + 1), "Error resizing dynamic array.");

	void *ptr = arr->ptr + arr->entry_size * r_idx;

	if (r_idx < arr->size) {
		arr->size++;
		for (size_t idx = r_idx + 1; idx < arr->size; idx++)
			dyn_swap(arr, idx - 1, idx);
	}

	if (data == NULL) memset(ptr, 0, arr->entry_size);
	else memcpy(ptr, data, arr->entry_size);

	if (r_idx >= arr->size) {
		arr->size = r_idx + 1;
	}

	return ptr;
}

void* dyn_append(dynarray_t *arr, void *data)
{
	assert(arr && arr->ptr);

	ERR_RET_NULL(dyn_resize(arr, arr->size + 1), "Error resizing dynamic array.");

	void *ptr = arr->ptr + arr->entry_size * arr->size++;
	if (data == NULL) memset(ptr, 0, arr->entry_size);
	else memcpy(ptr, data, arr->entry_size);

	return ptr;
}

void* dyn_get(dynarray_t *arr, int idx)
{
	assert(arr && arr->ptr);

	const size_t r_idx = GET_RIDX(arr->size, idx);
	if (r_idx >= arr->size) return NULL;

	return arr->ptr + r_idx * arr->entry_size;
}

int dyn_find(dynarray_t *arr, void *data)
{
	assert(arr && arr->ptr);

	if (data == NULL) return -1;
	for (size_t idx = 0; idx < arr->size; idx++) {
		if (memcmp(arr->ptr + idx * arr->entry_size, data, arr->entry_size) == 0) {
			return idx;
		}
	}

	return -1;
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

void dyn_remove(dynarray_t *arr, int idx, bool just_swap)
{
	assert(arr && arr->ptr);

	const size_t r_idx = GET_RIDX(arr->size, idx);
	if (just_swap) {
		dyn_swap(arr, r_idx, -1);
	}
	else {
		for (size_t idx = r_idx+1; idx < arr->size; idx++) {
			dyn_swap(arr, r_idx-1, idx);
		}
	}

	dyn_delete(arr, -1);
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
