#include "hash_state.h"
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <string.h>

//#define DEBUG
#include "debug.h"

const char *cmph_hash_names[] = { "jenkins", NULL };

hash_state_t *hash_state_new(CMPH_HASH hashfunc, cmph_uint32 hashsize)
{
	hash_state_t *state = NULL;
	switch (hashfunc)
	{
		case CMPH_HASH_JENKINS:
	  		DEBUGP("Jenkins function - %u\n", hashsize);
			state = (hash_state_t *)jenkins_state_new(hashsize);
	  		DEBUGP("Jenkins function created\n");
			break;
		default:
			assert(0);
	}
	state->hashfunc = hashfunc;
	return state;
}
cmph_uint32 hash(hash_state_t *state, const char *key, cmph_uint32 keylen)
{
	switch (state->hashfunc)
	{
		case CMPH_HASH_JENKINS:
			return jenkins_hash((jenkins_state_t *)state, key, keylen);
		default:
			assert(0);
	}
	assert(0);
	return 0;
}

void hash_vector(hash_state_t *state, const char *key, cmph_uint32 keylen, cmph_uint32 * hashes)
{
	switch (state->hashfunc)
	{
		case CMPH_HASH_JENKINS:
			jenkins_hash_vector((jenkins_state_t *)state, key, keylen, hashes);
			break;
		default:
			assert(0);
	}
}


void hash_state_dump(hash_state_t *state, char **buf, cmph_uint32 *buflen)
{
	char *algobuf;
	switch (state->hashfunc)
	{
		case CMPH_HASH_JENKINS:
			jenkins_state_dump((jenkins_state_t *)state, &algobuf, buflen);
			if (*buflen == UINT_MAX) return;
			break;
		default:
			assert(0);
	}
	*buf = (char *)malloc(strlen(cmph_hash_names[state->hashfunc]) + 1 + *buflen);
	memcpy(*buf, cmph_hash_names[state->hashfunc], strlen(cmph_hash_names[state->hashfunc]) + 1);
	DEBUGP("Algobuf is %u\n", *(cmph_uint32 *)algobuf);
	memcpy(*buf + strlen(cmph_hash_names[state->hashfunc]) + 1, algobuf, *buflen);
	*buflen  = (cmph_uint32)strlen(cmph_hash_names[state->hashfunc]) + 1 + *buflen;
	free(algobuf);
	return;
}

hash_state_t * hash_state_copy(hash_state_t *src_state)
{
	hash_state_t *dest_state = NULL;
	switch (src_state->hashfunc)
	{
		case CMPH_HASH_JENKINS:
			dest_state = (hash_state_t *)jenkins_state_copy((jenkins_state_t *)src_state);
			break;
		default:
			assert(0);
	}
	dest_state->hashfunc = src_state->hashfunc;
	return dest_state;
}

hash_state_t *hash_state_load(const char *buf, cmph_uint32 buflen)
{
	cmph_uint32 i;
	cmph_uint32 offset;
	CMPH_HASH hashfunc = CMPH_HASH_COUNT;
	for (i = 0; i < CMPH_HASH_COUNT; ++i)
	{
		if (strcmp(buf, cmph_hash_names[i]) == 0)
		{
			hashfunc = i;
			break;
		}
	}
	if (hashfunc == CMPH_HASH_COUNT) return NULL;
	offset = (cmph_uint32)strlen(cmph_hash_names[hashfunc]) + 1;
	switch (hashfunc)
	{
		case CMPH_HASH_JENKINS:
			return (hash_state_t *)jenkins_state_load(buf + offset, buflen - offset);
		default:
			return NULL;
	}
	return NULL;
}
void hash_state_destroy(hash_state_t *state)
{
	switch (state->hashfunc)
	{
		case CMPH_HASH_JENKINS:
			jenkins_state_destroy((jenkins_state_t *)state);
			break;
		default:
			assert(0);
	}
	return;
}
