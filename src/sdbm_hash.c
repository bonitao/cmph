#include "sdbm_hash.h"
#include <stdlib.h>

cmph_sdbm_state_t *cmph_sdbm_state_new()
{
	cmph_sdbm_state_t *state = (cmph_sdbm_state_t *)malloc(sizeof(cmph_sdbm_state_t));
	state->hashfunc = CMPH_HASH_SDBM;
	return state;
}

void cmph_sdbm_state_destroy(cmph_sdbm_state_t *state)
{
	free(state);
}

cmph_uint32 cmph_sdbm_hash(cmph_sdbm_state_t *state, const char *k, cmph_uint32 keylen)
{
	register cmph_uint32 hash = 0;
	const unsigned char *ptr = k;
	cmph_uint32 i = 0;

	while(i < keylen) {
		hash = *ptr + (hash << 6) + (hash << 16) - hash;
		++ptr, ++i;
	}
	return hash;
}


void cmph_sdbm_state_dump(cmph_sdbm_state_t *state, char **buf, cmph_uint32 *buflen)
{
	*buf = NULL;
	*buflen = 0;
	return;
}

cmph_sdbm_state_t *cmph_sdbm_state_load(const char *buf, cmph_uint32 buflen)
{
	cmph_sdbm_state_t *state = (cmph_sdbm_state_t *)malloc(sizeof(cmph_sdbm_state_t));
	state->hashfunc = CMPH_HASH_SDBM;
	return state;
}
