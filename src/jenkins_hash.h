#ifndef __JEKINS_HASH_H__
#define __JEKINS_HASH_H__

#include "hash.h"

typedef struct cmph__jenkins_state_t
{
	CMPH_HASH hashfunc;
	cmph_uint32 seed;
	cmph_uint32 nbits;
	cmph_uint32 size;
} cmph_jenkins_state_t;
	
cmph_jenkins_state_t *cmph_jenkins_state_new(cmph_uint32 size); //size of hash table
cmph_uint32 cmph_jenkins_hash(cmph_jenkins_state_t *state, const char *k, cmph_uint32 keylen);
void cmph_jenkins_state_dump(cmph_jenkins_state_t *state, char **buf, cmph_uint32 *buflen);
cmph_jenkins_state_t *cmph_jenkins_state_load(const char *buf, cmph_uint32 buflen);
void cmph_jenkins_state_destroy(cmph_jenkins_state_t *state);

#endif
