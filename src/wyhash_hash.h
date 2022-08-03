#ifndef __WYHASH_HASH_H__
#define __WYHASH_HASH_H__

#include "hash.h"

typedef struct __wyhash_state_t
{
	CMPH_HASH hashfunc;
	cmph_uint32 seed;
} wyhash_state_t;
	
wyhash_state_t *wyhash_state_new(cmph_uint32 size); //size of hash table

/** \fn cmph_uint32 wyhash_hash(wyhash_state_t *state, const char *k, cmph_uint32 keylen);
 *  \param state is a pointer to a wyhash_state_t structure
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \return an integer that represents a hash value of 32 bits.
 */
cmph_uint32 wyhash_hash(wyhash_state_t *state, const char *k, cmph_uint32 keylen);

/** \fn void wyhash_hash_vector_(wyhash_state_t *state, const char *k, cmph_uint32 keylen, cmph_uint32 * hashes);
 *  \param state is a pointer to a wyhash_state_t structure
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \param hashes is a pointer to a memory large enough to fit three 32-bit integers.
 */
void wyhash_hash_vector_(wyhash_state_t *state, const char *k, cmph_uint32 keylen, cmph_uint32 * hashes);

void wyhash_state_dump(wyhash_state_t *state, char **buf, cmph_uint32 *buflen);
wyhash_state_t *wyhash_state_copy(wyhash_state_t *src_state);
wyhash_state_t *wyhash_state_load(const char *buf, cmph_uint32 buflen);
void wyhash_state_destroy(wyhash_state_t *state);

/** \fn void wyhash_state_pack(wyhash_state_t *state, void *wyhash_packed);
 *  \brief Support the ability to pack a wyhash function into a preallocated contiguous memory space pointed by wyhash_packed.
 *  \param state points to the wyhash function
 *  \param wyhash_packed pointer to the contiguous memory area used to store the wyhash function. The size of wyhash_packed must be at least wyhash_state_packed_size() 
 */
void wyhash_state_pack(wyhash_state_t *state, void *wyhash_packed);

/** \fn cmph_uint32 wyhash_state_packed_size();
 *  \brief Return the amount of space needed to pack a wyhash function.
 *  \return the size of the packed function or zero for failures
 */ 
cmph_uint32 wyhash_state_packed_size(void);


/** \fn cmph_uint32 wyhash_hash_packed(void *wyhash_packed, const char *k, cmph_uint32 keylen);
 *  \param wyhash_packed is a pointer to a contiguous memory area
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \return an integer that represents a hash value of 32 bits.
 */
cmph_uint32 wyhash_hash_packed(void *wyhash_packed, const char *k, cmph_uint32 keylen);

/** \fn wyhash_hash_vector_packed(void *wyhash_packed, const char *k, cmph_uint32 keylen, cmph_uint32 * hashes);
 *  \param wyhash_packed is a pointer to a contiguous memory area
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \param hashes is a pointer to a memory large enough to fit three 32-bit integers.
 */
void wyhash_hash_vector_packed(void *wyhash_packed, const char *k, cmph_uint32 keylen, cmph_uint32 * hashes);

#endif
