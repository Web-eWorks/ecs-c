#ifndef ECS_HASH_H
#define ECS_HASH_H

#include <stddef.h>

typedef unsigned int hash_t;
typedef struct hashtable_t* hashtable_t;

hash_t hash_string(const char *str);
hash_t hash_bytes(unsigned char *bytes, size_t len);

// TODO: hashtable functions

#endif