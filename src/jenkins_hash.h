#ifndef __JEKINS_HASH_H__
#define __JEKINS_HASH_H__

#include "hash.h"

typedef struct __jenkins_state_t
{
	CMPH_HASH hashfunc;
	cmph_uint32 seed;
} jenkins_state_t;
	
jenkins_state_t *jenkins_state_new(cmph_uint32 size); //size of hash table
cmph_uint32 jenkins_hash(jenkins_state_t *state, const char *k, cmph_uint32 keylen);
void jenkins_state_dump(jenkins_state_t *state, char **buf, cmph_uint32 *buflen);
jenkins_state_t *jenkins_state_copy(jenkins_state_t *src_state);
jenkins_state_t *jenkins_state_load(const char *buf, cmph_uint32 buflen);
void jenkins_state_destroy(jenkins_state_t *state);

#endif
