#include "hash_state.h"
#include <stdlib.h>
#include <assert.h>
#include <limits.h>
#include <string.h>

//#define DEBUG
#include "debug.h"

const char *hash_names[] = { "jenkins", "djb2", "sdbm", "fnv", "glib", "pjw", NULL };

hash_state_t *hash_state_new(CMPH_HASH hashfunc, uint32 hashsize)
{
	hash_state_t *state = NULL;
	switch (hashfunc)
	{
		case HASH_JENKINS:
	  		DEBUGP("Jenkins function - %u\n", hashsize);
			state = (hash_state_t *)jenkins_state_new(hashsize);
	  		DEBUGP("Jenkins function created\n");
			break;
		case HASH_DJB2:
			state = (hash_state_t *)djb2_state_new();
			break;
		case HASH_SDBM:
			state = (hash_state_t *)sdbm_state_new();
			break;
		case HASH_FNV:
			state = (hash_state_t *)fnv_state_new();
			break;
		default:
			assert(0);
	}
	state->hashfunc = hashfunc;
	return state;
}
uint32 hash(hash_state_t *state, const char *key, uint32 keylen)
{
	switch (state->hashfunc)
	{
		case HASH_JENKINS:
			return jenkins_hash((jenkins_state_t *)state, key, keylen);
		case HASH_DJB2:
			return djb2_hash((djb2_state_t *)state, key, keylen);
		case HASH_SDBM:
			return sdbm_hash((sdbm_state_t *)state, key, keylen);
		case HASH_FNV:
			return fnv_hash((fnv_state_t *)state, key, keylen);
		default:
			assert(0);
	}
	assert(0);
	return 0;
}

void hash_state_dump(hash_state_t *state, char **buf, uint32 *buflen)
{
	char *algobuf;
	switch (state->hashfunc)
	{
		case HASH_JENKINS:
			jenkins_state_dump((jenkins_state_t *)state, &algobuf, buflen);
			if (*buflen == UINT_MAX) return;
			break;
		case HASH_DJB2:
			djb2_state_dump((djb2_state_t *)state, &algobuf, buflen);
			if (*buflen == UINT_MAX) return;
			break;
		case HASH_SDBM:
			sdbm_state_dump((sdbm_state_t *)state, &algobuf, buflen);
			if (*buflen == UINT_MAX) return;
			break;
		case HASH_FNV:
			fnv_state_dump((fnv_state_t *)state, &algobuf, buflen);
			if (*buflen == UINT_MAX) return;
			break;
		default:
			assert(0);
	}
	*buf = malloc(strlen(hash_names[state->hashfunc]) + 1 + *buflen);
	memcpy(*buf, hash_names[state->hashfunc], strlen(hash_names[state->hashfunc]) + 1);
	DEBUGP("Algobuf is %u\n", *(uint32 *)algobuf);
	memcpy(*buf + strlen(hash_names[state->hashfunc]) + 1, algobuf, *buflen);
	*buflen  = (uint32)strlen(hash_names[state->hashfunc]) + 1 + *buflen;
	free(algobuf);
	return;
}

hash_state_t *hash_state_load(const char *buf, uint32 buflen)
{
	uint32 i;
	uint32 offset;
	CMPH_HASH hashfunc = HASH_COUNT;
	for (i = 0; i < HASH_COUNT; ++i)
	{
		if (strcmp(buf, hash_names[i]) == 0)
		{
			hashfunc = i;
			break;
		}
	}
	if (hashfunc == HASH_COUNT) return NULL;
	offset = (uint32)strlen(hash_names[hashfunc]) + 1;
	switch (hashfunc)
	{
		case HASH_JENKINS:
			return (hash_state_t *)jenkins_state_load(buf + offset, buflen - offset);
		case HASH_DJB2:
			return (hash_state_t *)djb2_state_load(buf + offset, buflen - offset);
		case HASH_SDBM:
			return (hash_state_t *)sdbm_state_load(buf + offset, buflen - offset);
		case HASH_FNV:
			return (hash_state_t *)fnv_state_load(buf + offset, buflen - offset);
		default:
			return NULL;
	}
	return NULL;
}
void hash_state_destroy(hash_state_t *state)
{
	switch (state->hashfunc)
	{
		case HASH_JENKINS:
			jenkins_state_destroy((jenkins_state_t *)state);
			break;
		case HASH_DJB2:
			djb2_state_destroy((djb2_state_t *)state);
			break;
		case HASH_SDBM:
			sdbm_state_destroy((sdbm_state_t *)state);
			break;
		case HASH_FNV:
			fnv_state_destroy((fnv_state_t *)state);
			break;
		default:
			assert(0);
	}
	return;
}
