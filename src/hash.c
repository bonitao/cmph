#include "hash_state.h"
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <string.h>

//#define DEBUG
#include "debug.h"

const char *cmph_hash_names[] = { "jenkins", "djb2", "sdbm", "fnv", "glib", "pjw", NULL };

cmph_hash_state_t *cmph_hash_state_new(CMPH_HASH hashfunc, cmph_uint32 hashsize)
{
	cmph_hash_state_t *state = NULL;
	switch (hashfunc)
	{
		case CMPH_HASH_JENKINS:
	  		DEBUGP("Jenkins function - %u\n", hashsize);
			state = (cmph_hash_state_t *)cmph_jenkins_state_new(hashsize);
	  		DEBUGP("Jenkins function created\n");
			break;
		case CMPH_HASH_DJB2:
			state = (cmph_hash_state_t *)cmph_djb2_state_new();
			break;
		case CMPH_HASH_SDBM:
			state = (cmph_hash_state_t *)cmph_sdbm_state_new();
			break;
		case CMPH_HASH_FNV:
			state = (cmph_hash_state_t *)cmph_fnv_state_new();
			break;
		default:
			assert(0);
	}
	state->hashfunc = hashfunc;
	return state;
}
cmph_uint32 cmph_hash(cmph_hash_state_t *state, const char *key, cmph_uint32 keylen)
{
	switch (state->hashfunc)
	{
		case CMPH_HASH_JENKINS:
			return cmph_jenkins_hash((cmph_jenkins_state_t *)state, key, keylen);
		case CMPH_HASH_DJB2:
			return cmph_djb2_hash((cmph_djb2_state_t *)state, key, keylen);
		case CMPH_HASH_SDBM:
			return cmph_sdbm_hash((cmph_sdbm_state_t *)state, key, keylen);
		case CMPH_HASH_FNV:
			return cmph_fnv_hash((cmph_fnv_state_t *)state, key, keylen);
		default:
			assert(0);
	}
	assert(0);
	return 0;
}

void cmph_hash_state_dump(cmph_hash_state_t *state, char **buf, cmph_uint32 *buflen)
{
	char *algobuf;
	switch (state->hashfunc)
	{
		case CMPH_HASH_JENKINS:
			cmph_jenkins_state_dump((cmph_jenkins_state_t *)state, &algobuf, buflen);
			if (*buflen == UINT_MAX) return;
			break;
		case CMPH_HASH_DJB2:
			cmph_djb2_state_dump((cmph_djb2_state_t *)state, &algobuf, buflen);
			if (*buflen == UINT_MAX) return;
			break;
		case CMPH_HASH_SDBM:
			cmph_sdbm_state_dump((cmph_sdbm_state_t *)state, &algobuf, buflen);
			if (*buflen == UINT_MAX) return;
			break;
		case CMPH_HASH_FNV:
			cmph_fnv_state_dump((cmph_fnv_state_t *)state, &algobuf, buflen);
			if (*buflen == UINT_MAX) return;
			break;
		default:
			assert(0);
	}
	*buf = malloc(strlen(cmph_hash_names[state->hashfunc]) + 1 + *buflen);
	memcpy(*buf, cmph_hash_names[state->hashfunc], strlen(cmph_hash_names[state->hashfunc]) + 1);
	DEBUGP("Algobuf is %u\n", *(cmph_uint32 *)algobuf);
	memcpy(*buf + strlen(cmph_hash_names[state->hashfunc]) + 1, algobuf, *buflen);
	*buflen  = (cmph_uint32)strlen(cmph_hash_names[state->hashfunc]) + 1 + *buflen;
	free(algobuf);
	return;
}

cmph_hash_state_t *cmph_hash_state_load(const char *buf, cmph_uint32 buflen)
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
			return (cmph_hash_state_t *)cmph_jenkins_state_load(buf + offset, buflen - offset);
		case CMPH_HASH_DJB2:
			return (cmph_hash_state_t *)cmph_djb2_state_load(buf + offset, buflen - offset);
		case CMPH_HASH_SDBM:
			return (cmph_hash_state_t *)cmph_sdbm_state_load(buf + offset, buflen - offset);
		case CMPH_HASH_FNV:
			return (cmph_hash_state_t *)cmph_fnv_state_load(buf + offset, buflen - offset);
		default:
			return NULL;
	}
	return NULL;
}
void cmph_hash_state_destroy(cmph_hash_state_t *state)
{
	switch (state->hashfunc)
	{
		case CMPH_HASH_JENKINS:
			cmph_jenkins_state_destroy((cmph_jenkins_state_t *)state);
			break;
		case CMPH_HASH_DJB2:
			cmph_djb2_state_destroy((cmph_djb2_state_t *)state);
			break;
		case CMPH_HASH_SDBM:
			cmph_sdbm_state_destroy((cmph_sdbm_state_t *)state);
			break;
		case CMPH_HASH_FNV:
			cmph_fnv_state_destroy((cmph_fnv_state_t *)state);
			break;
		default:
			assert(0);
	}
	return;
}
