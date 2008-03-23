#ifndef __CMPH_HASH_H__
#define __CMPH_HASH_H__

#include "cmph_types.h"

typedef union __hash_state_t hash_state_t;

hash_state_t *hash_state_new(CMPH_HASH, cmph_uint32 hashsize);

/** \fn cmph_uint32 hash(hash_state_t *state, const char *key, cmph_uint32 keylen);
 *  \param state is a pointer to a hash_state_t structure
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \return an integer that represents a hash value of 32 bits.
 */
cmph_uint32 hash(hash_state_t *state, const char *key, cmph_uint32 keylen);

/** \fn void hash_vector(hash_state_t *state, const char *key, cmph_uint32 keylen, cmph_uint32 * hashes);
 *  \param state is a pointer to a hash_state_t structure
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \param hashes is a pointer to a memory large enough to fit three 32-bit integers.
 */
void hash_vector(hash_state_t *state, const char *key, cmph_uint32 keylen, cmph_uint32 * hashes);

void hash_state_dump(hash_state_t *state, char **buf, cmph_uint32 *buflen);

hash_state_t * hash_state_copy(hash_state_t *src_state);

hash_state_t *hash_state_load(const char *buf, cmph_uint32 buflen);

void hash_state_destroy(hash_state_t *state);

#endif
