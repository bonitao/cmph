#include "fnv_hash.h"
#include <stdlib.h>

cmph_fnv_state_t *cmph_fnv_state_new()
{
	cmph_fnv_state_t *state = (cmph_fnv_state_t *)malloc(sizeof(cmph_fnv_state_t));
	state->hashfunc = CMPH_HASH_FNV;
	return state;
}

void cmph_fnv_state_destroy(cmph_fnv_state_t *state)
{
	free(state);
}

cmph_uint32 cmph_fnv_hash(cmph_fnv_state_t *state, const char *k, cmph_uint32 keylen)
{
	const unsigned char *bp = (const unsigned char *)k;	
	const unsigned char *be = bp + keylen;	
	static unsigned int hval = 0;	

	while (bp < be) 
	{
		
		//hval *= 0x01000193; good for non-gcc compiler
		hval += (hval << 1) + (hval << 4) + (hval << 7) + (hval << 8) + (hval << 24); //good for gcc

		hval ^= *bp++;
	}
	return hval;
}


void cmph_fnv_state_dump(cmph_fnv_state_t *state, char **buf, cmph_uint32 *buflen)
{
	*buf = NULL;
	*buflen = 0;
	return;
}

cmph_fnv_state_t *cmph_fnv_state_load(const char *buf, cmph_uint32 buflen)
{
	cmph_fnv_state_t *state = (cmph_fnv_state_t *)malloc(sizeof(cmph_fnv_state_t));
	state->hashfunc = CMPH_HASH_FNV;
	return state;
}
