// hash.h

#ifndef ECS_HASH_H
#define ECS_HASH_H

#include <stddef.h>
#include <stdint.h>

/*
    The hash type. By default, hashes are stored as 32-bit unsigned integers,
    with a maximum value of ~4 billion entries per hash table.

    In the specific use case of this ECS, an average system will run out of
    memory before breaching the 32-bit limit. At the point a 64-bit hash is
    required,
*/
typedef uint32_t hash_t;

hash_t hash_string(const char *str);
hash_t hash_bytes(const char *bytes, const size_t len);

#endif
