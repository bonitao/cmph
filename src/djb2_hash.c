#include "djb2_hash.h"
#include <stdlib.h>

cmph_djb2_state_t *cmph_djb2_state_new()
{
	cmph_djb2_state_t *state = (cmph_djb2_state_t *)malloc(sizeof(cmph_djb2_state_t));
	state->hashfunc = CMPH_HASH_DJB2;
	return state;
}

void cmph_djb2_state_destroy(cmph_djb2_state_t *state)
{
	free(state);
}

cmph_uint32 cmph_djb2_hash(cmph_djb2_state_t *state, const char *k, cmph_uint32 keylen)
{
	register cmph_uint32 hash = 5381;
	const unsigned char *ptr = k;
	cmph_uint32 i = 0;
	while (i < keylen) 
	{
		hash = hash*33 ^ *ptr;
		++ptr, ++i;
	}
	return hash;
}


void cmph_djb2_state_dump(cmph_djb2_state_t *state, char **buf, cmph_uint32 *buflen)
{
	*buf = NULL;
	*buflen = 0;
	return;
}

cmph_djb2_state_t *cmph_djb2_state_load(const char *buf, cmph_uint32 buflen)
{
	cmph_djb2_state_t *state = (cmph_djb2_state_t *)malloc(sizeof(cmph_djb2_state_t));
	state->hashfunc = CMPH_HASH_DJB2;
	return state;
}
