#ifndef __CMPH_HASH_H__
#define __CMPH_HASH_H__

#include "cmph_types.h"

typedef union cmph__hash_state_t cmph_hash_state_t;

cmph_hash_state_t *cmph_hash_state_new(CMPH_HASH, cmph_uint32 hashsize);
cmph_uint32 cmph_hash(cmph_hash_state_t *state, const char *key, cmph_uint32 keylen);
void cmph_hash_state_dump(cmph_hash_state_t *state, char **buf, cmph_uint32 *buflen);
cmph_hash_state_t *cmph_hash_state_load(const char *buf, cmph_uint32 buflen);
void cmph_hash_state_destroy(cmph_hash_state_t *state);

#endif
