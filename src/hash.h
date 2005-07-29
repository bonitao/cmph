#ifndef __CMPH_HASH_H__
#define __CMPH_HASH_H__

#include "cmph_types.h"

typedef union __hash_state_t hash_state_t;

hash_state_t *hash_state_new(CMPH_HASH, cmph_uint32 hashsize);
cmph_uint32 hash(hash_state_t *state, const char *key, cmph_uint32 keylen);
void hash_state_dump(hash_state_t *state, char **buf, cmph_uint32 *buflen);
hash_state_t * hash_state_copy(hash_state_t *src_state);
hash_state_t *hash_state_load(const char *buf, cmph_uint32 buflen);
void hash_state_destroy(hash_state_t *state);

#endif
