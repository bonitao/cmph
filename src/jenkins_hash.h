#ifndef __JEKINS_HASH_H__
#define __JEKINS_HASH_H__

#include "hash.h"

typedef struct __jenkins_state_t
{
	CMPH_HASH hashfunc;
	uint32 seed;
	uint32 nbits;
	uint32 size;
} jenkins_state_t;
	
jenkins_state_t *jenkins_state_new(uint32 size); //size of hash table
uint32 jenkins_hash(jenkins_state_t *state, const char *k, uint32 keylen);
void jenkins_state_dump(jenkins_state_t *state, char **buf, uint32 *buflen);
jenkins_state_t *jenkins_state_load(const char *buf, uint32 buflen);
void jenkins_state_destroy(jenkins_state_t *state);

#endif
