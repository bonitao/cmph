#ifndef __CRC32_HASH_H__
#define __CRC32_HASH_H__

#include "hash.h"

typedef struct __crc32_state_t
{
	CMPH_HASH hashfunc;
	cmph_uint32 seed;
} crc32_state_t;

crc32_state_t *crc32_state_new(cmph_uint32 size); //size of hash table

/** \fn cmph_uint32 crc32_hash(crc32_state_t *state, const char *k, cmph_uint32 keylen);
 *  \param state is a pointer to a crc32_state_t structure
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \return an integer that represents a hash value of 32 bits.
 */
cmph_uint32 crc32_hash(crc32_state_t *state, const char *k, cmph_uint32 keylen);

/** \fn void crc32_hash_vector_(crc32_state_t *state, const char *k, cmph_uint32 keylen, cmph_uint32 * hashes);
 *  \param state is a pointer to a crc32_state_t structure
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \param hashes is a pointer to a memory large enough to fit three 32-bit integers.
 */
void crc32_hash_vector_(crc32_state_t *state, const char *k, cmph_uint32 keylen, cmph_uint32 * hashes);

void crc32_state_dump(crc32_state_t *state, char **buf, cmph_uint32 *buflen);
crc32_state_t *crc32_state_copy(crc32_state_t *src_state);
crc32_state_t *crc32_state_load(const char *buf, cmph_uint32 buflen);
void crc32_state_destroy(crc32_state_t *state);

/** \fn void crc32_state_pack(crc32_state_t *state, void *crc32_packed);
 *  \brief Support the ability to pack a crc32 function into a preallocated contiguous memory space pointed by crc32_packed.
 *  \param state points to the crc32 function
 *  \param crc32_packed pointer to the contiguous memory area used to store the crc32 function. The size of crc32_packed must be at least crc32_state_packed_size()
 */
void crc32_state_pack(crc32_state_t *state, void *crc32_packed);

/** \fn cmph_uint32 crc32_state_packed_size();
 *  \brief Return the amount of space needed to pack a crc32 function.
 *  \return the size of the packed function or zero for failures
 */
cmph_uint32 crc32_state_packed_size(void);


/** \fn cmph_uint32 crc32_hash_packed(void *crc32_packed, const char *k, cmph_uint32 keylen);
 *  \param crc32_packed is a pointer to a contiguous memory area
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \return an integer that represents a hash value of 32 bits.
 */
cmph_uint32 crc32_hash_packed(void *crc32_packed, const char *k, cmph_uint32 keylen);

/** \fn crc32_hash_vector_packed(void *crc32_packed, const char *k, cmph_uint32 keylen, cmph_uint32 * hashes);
 *  \param crc32_packed is a pointer to a contiguous memory area
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \param hashes is a pointer to a memory large enough to fit three 32-bit integers.
 */
void crc32_hash_vector_packed(void *crc32_packed, const char *k, cmph_uint32 keylen, cmph_uint32 * hashes);

#endif
