#ifndef __FNV_HASH_H__
#define __FNV_HASH_H__

#include "hash.h"

typedef struct cmph__fnv_state_t
{
	CMPH_HASH hashfunc;
} cmph_fnv_state_t;

cmph_fnv_state_t *cmph_fnv_state_new();
cmph_uint32 cmph_fnv_hash(cmph_fnv_state_t *state, const char *k, cmph_uint32 keylen);
void cmph_fnv_state_dump(cmph_fnv_state_t *state, char **buf, cmph_uint32 *buflen);
cmph_fnv_state_t *cmph_fnv_state_load(const char *buf, cmph_uint32 buflen);
void cmph_fnv_state_destroy(cmph_fnv_state_t *state);

#endif
