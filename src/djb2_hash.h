#ifndef __DJB2_HASH_H__
#define __DJB2_HASH_H__

#include "hash.h"

typedef struct cmph__djb2_state_t
{
	CMPH_HASH hashfunc;
} cmph_djb2_state_t;

cmph_djb2_state_t *cmph_djb2_state_new();
cmph_uint32 cmph_djb2_hash(cmph_djb2_state_t *state, const char *k, cmph_uint32 keylen);
void cmph_djb2_state_dump(cmph_djb2_state_t *state, char **buf, cmph_uint32 *buflen);
cmph_djb2_state_t *cmph_djb2_state_load(const char *buf, cmph_uint32 buflen);
void cmph_djb2_state_destroy(cmph_djb2_state_t *state);

#endif
