
#include "hash.h"
#include "zlib.h"

#include <string.h>

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
