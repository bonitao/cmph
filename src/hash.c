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
			jenkins_hash_vector_((jenkins_state_t *)state, key, keylen, hashes);
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

/** \fn void hash_state_pack(hash_state_t *state, void *hash_packed);
 *  \brief Support the ability to pack a hash function into a preallocated contiguous memory space pointed by hash_packed.
 *  \param state points to the hash function
 *  \param hash_packed pointer to the contiguous memory area used to store the hash function. The size of hash_packed must be at least hash_state_packed_size() 
 */
void hash_state_pack(hash_state_t *state, void *hash_packed)
{
	cmph_uint32 * ptr = (cmph_uint32 *)hash_packed;
	cmph_uint32 * ptr_size = ptr++;

	// Reserve space for the hash function size
	*ptr_size = 0;

	// Pack the hash function type
	*ptr++ = state->hashfunc;

	switch (state->hashfunc)
	{
		case CMPH_HASH_JENKINS:
			// pack the jenkins hash function			
			jenkins_state_pack((jenkins_state_t *)state, ptr);
			*ptr_size = sizeof(cmph_uint32) + sizeof(CMPH_HASH)  + jenkins_state_packed_size();
			break;
		default:
			assert(0);
	}
	return;	
}

/** \fn cmph_uint32 hash_state_packed_size(hash_state_t *state);
 *  \brief Return the amount of space needed to pack a hash function.
 *  \param state points to a hash function
 *  \return the size of the packed function or zero for failures
 */ 
cmph_uint32 hash_state_packed_size(hash_state_t *state)
{
	cmph_uint32 size = sizeof(cmph_uint32) + sizeof(CMPH_HASH);
	switch (state->hashfunc)
	{
		case CMPH_HASH_JENKINS:
			size += jenkins_state_packed_size();
			break;
		default:
			assert(0);
	}
	return size;
}


/** \fn cmph_uint32 hash_packed(void *hash_packed, const char *k, cmph_uint32 keylen)
 *  \param hash_packed is a pointer to a contiguous memory area
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \return an integer that represents a hash value of 32 bits.
 */
cmph_uint32 hash_packed(void *hash_packed, const char *k, cmph_uint32 keylen)
{
	register cmph_uint32 * ptr = (((cmph_uint32 *) hash_packed) + 1);
		
	register CMPH_HASH hashfunc = *ptr++;

	switch (hashfunc)
	{
		case CMPH_HASH_JENKINS:
			return jenkins_hash_packed(ptr, k, keylen);
		default:
			assert(0);
	}
	assert(0);
	return 0;
}

/** \fn hash_vector_packed(void *hash_packed, const char *k, cmph_uint32 keylen, cmph_uint32 * hashes);
 *  \param hash_packed is a pointer to a contiguous memory area
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \param hashes is a pointer to a memory large enough to fit three 32-bit integers.
 */
void hash_vector_packed(void *hash_packed, const char *k, cmph_uint32 keylen, cmph_uint32 * hashes)
{
	cmph_uint32 * ptr = (((cmph_uint32 *) hash_packed) + 1);
		
	CMPH_HASH hashfunc = *ptr++;
	switch (hashfunc)
	{
		case CMPH_HASH_JENKINS:
			jenkins_hash_vector_packed(ptr, k, keylen, hashes);
			break;
		default:
			assert(0);
	}
}
