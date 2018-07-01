#include "hash.h"
#include "zlib.h"
#include <string.h>

struct hashtable_t {

};

hash_t hash_string(const char *string) {
	uLong crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, string, strlen(string));
	return crc;
}