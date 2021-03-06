// dynarray.h - dynamic arrays

#ifndef ECS_DYNARRAY_H
#define ECS_DYNARRAY_H

#include <stdbool.h>
#include <stdlib.h>

typedef struct {
	void *ptr;
	size_t size;
	size_t entry_size;
	size_t capacity;
} dynarray_t;

/*
	Perform in-place initialization of a dynamic array.
	Will call dyn_free if the array is already initialized.
*/
bool dyn_alloc(dynarray_t *arr, size_t size, size_t entry_size);
/*
	Free a dynamic array's storage.
	Will make pointers to stored items invalid.
*/
void dyn_free(dynarray_t *arr);

/*
	Add a new item to the end of the array.
*/
void* dyn_append(dynarray_t *arr, void *data);

/*
	Insert a new item into the array, moving existing items out of the way.
	If idx is negative, the item is inserted at arr->size - idx. If idx is
	greater than the index of the last item in the array, it will extend the
	array, potentially creating holes.

	To append to an array, use:
		dyn_append(arr, data);

	If idx is already occupied, this function will increase the size of the array
	and shuffle the entry and all following entries towards the end by one index.

	If data is NULL, clears the allocated memory with zeros.
*/
void* dyn_insert(dynarray_t *arr, int idx, void *data);

/*
	Get an item from the array.
	A negative idx walks backwards from the end of the array.
*/
void* dyn_get(dynarray_t *arr, int idx);

/*
	Find an item in the array. Returns -1 if that item isn't present or if data is
	NULL.

	Care should be taken to pass a valid pointer to this function.
*/
int dyn_find(dynarray_t *arr, void *data);

/*
	Remove an item from the index.
	A negative index walks backwards from the end of the array.

	'Deleting' an item simply zeros the memory assigned to it, and does not
	re-arrange the array. Use dyn_swap() to close holes in the array.

	Deleting the last item in the array will properly decrement the size counter.
*/
void dyn_delete(dynarray_t *arr, int idx);

/*
	Swap the position of two items in the array.
	A negative index walks back from the end.

	To delete an item and close holes in the array, there are two options.

	Option a (if you don't care about array order):

		dyn_swap(arr, item_idx, -1);
		dyn_delete(arr, -1);

	Option b (if you care about array order):

		for (int i = item_idx+1; i < arr->size; i++) {
			dyn_swap(arr, i-1, i);
		}
		dyn_delete(arr, -1);

*/
bool dyn_swap(dynarray_t *arr, int idx_a, int idx_b);

/*
	Removes an item in the array and closes gaps.

	If just_swap is true, this function swaps the removed element and the last
	element, saving time but destroying array order.
	Otherwise, this function preserves the order of the array.

*/
void dyn_remove(dynarray_t *arr, int idx, bool just_swap);

/*
	Reserve space for at least `newcap` more slots in the array beyond the last
	filled slot.

	Only grows the array if it does not already have enough capacity.
*/
bool dyn_reserve(dynarray_t *arr, size_t newcap);

/*
	Resize the array so it has exactly `newcap` slots. If `newcap` is less than
	`arr->size`, it will be clamped to the end of the array.
*/
bool dyn_resize(dynarray_t *arr, size_t newcap);

#define DYN_FOR(arr, start) for (size_t idx = start; idx < (arr).size; idx++)

#endif
