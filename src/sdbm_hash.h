#ifndef __SDBM_HASH_H__
#define __SDBM_HASH_H__

#include "hash.h"

typedef struct cmph__sdbm_state_t
{
	CMPH_HASH hashfunc;
} cmph_sdbm_state_t;

cmph_sdbm_state_t *cmph_sdbm_state_new();
cmph_uint32 cmph_sdbm_hash(cmph_sdbm_state_t *state, const char *k, cmph_uint32 keylen);
void cmph_sdbm_state_dump(cmph_sdbm_state_t *state, char **buf, cmph_uint32 *buflen);
cmph_sdbm_state_t *cmph_sdbm_state_load(const char *buf, cmph_uint32 buflen);
void cmph_sdbm_state_destroy(cmph_sdbm_state_t *state);

#endif
