#include "wyhash_hash.h"
#include <stdlib.h>
#include <stdint.h>
#ifdef WIN32
#define _USE_MATH_DEFINES //For M_LOG2E
#endif
#include <math.h>
#include <limits.h>
#include <string.h>
#include "wyhash.h"

//#define DEBUG
#include "debug.h"

#define hashsize(n) ((cmph_uint32)1<<(n))
#define hashmask(n) (hashsize(n)-1)

/*
   --------------------------------------------------------------------
   hash() -- hash a variable-length key into a 32-bit value
k       : the key (the unaligned variable-length array of bytes)
len     : the length of the key, counting by bytes
initval : can be any 4-byte value
--------------------------------------------------------------------
 */
wyhash_state_t *wyhash_state_new(cmph_uint32 size) //size of hash table
{
	wyhash_state_t *state = (wyhash_state_t *)malloc(sizeof(wyhash_state_t));
        if (!state) return NULL;
	DEBUGP("Initializing wyhash hash\n");
	state->seed = ((cmph_uint32)rand() % size);
	return state;
}
void wyhash_state_destroy(wyhash_state_t *state)
{
	free(state);
}


static inline void __wyhash_hash_vector(cmph_uint32 seed, const unsigned char *k, cmph_uint32 len, cmph_uint32 * hashes)
{
        uint64_t *hashes64 = (uint64_t*) hashes;
        const uint64_t secret[4] = { _wyp[0],_wyp[1],_wyp[2],_wyp[3] };
        const uint8_t *p = (const uint8_t *)k;
        seed ^= secret[0];
        uint64_t a, b;
        if(_likely_(len<=16)){
                if(_likely_(len>=4)){
                        a=(_wyr4(p)<<32)|_wyr4(p+((len>>3)<<2));
                        b=(_wyr4(p+len-4)<<32)|_wyr4(p+len-4-((len>>3)<<2));
                }
                else if(_likely_(len>0)){
                        a=_wyr3(p,len); b=0;
                }
                else a=b=0;
        }
        else {
                size_t i=len; 
                if(_unlikely_(i>48)){
                        uint64_t see1=seed, see2=seed;
                        do {
                                seed=_wymix(_wyr8(p)^secret[1],_wyr8(p+8)^seed);
                                see1=_wymix(_wyr8(p+16)^secret[2],_wyr8(p+24)^see1);
                                see2=_wymix(_wyr8(p+32)^secret[3],_wyr8(p+40)^see2);
                                p+=48; i-=48;
                        } while (_likely_(i>48));
                        seed^=see1^see2;
                }
                while(_unlikely_(i>16)){
                        seed=_wymix(_wyr8(p)^secret[1],_wyr8(p+8)^seed);
                        i-=16; p+=16;
                }
                a=_wyr8(p+i-16);  b=_wyr8(p+i-8);
        }
        hashes64[0] = _wymix(secret[1]^len,_wymix(a^secret[1], b^seed));
        hashes64[1] = _wymix(b^secret[1], a^seed);

        //wyhash(k, keylen, seed, (cmph_uint64*)hashes); // not enough hashes
}

cmph_uint32 wyhash_hash(wyhash_state_t *state, const char *k, cmph_uint32 keylen)
{
	cmph_uint32 hashes[3];
	__wyhash_hash_vector(state->seed, (const unsigned char*)k, keylen, hashes);
	return hashes[2];
}

void wyhash_hash_vector_(wyhash_state_t *state, const char *k, cmph_uint32 keylen, cmph_uint32 * hashes)
{
	__wyhash_hash_vector(state->seed, (const unsigned char*)k, keylen, hashes);
}

void wyhash_state_dump(wyhash_state_t *state, char **buf, cmph_uint32 *buflen)
{
	*buflen = sizeof(cmph_uint32);
	*buf = (char *)malloc(sizeof(cmph_uint32));
	if (!*buf)
	{
		*buflen = UINT_MAX;
		return;
	}
	memcpy(*buf, &(state->seed), sizeof(cmph_uint32));
	DEBUGP("Dumped wyhash state with seed %u\n", state->seed);
	return;
}

wyhash_state_t *wyhash_state_copy(wyhash_state_t *src_state)
{
	wyhash_state_t *dest_state = (wyhash_state_t *)malloc(sizeof(wyhash_state_t));
	dest_state->hashfunc = src_state->hashfunc;
	dest_state->seed = src_state->seed;
	return dest_state;
}

wyhash_state_t *wyhash_state_load(const char *buf, cmph_uint32 buflen)
{
	wyhash_state_t *state = (wyhash_state_t *)malloc(sizeof(wyhash_state_t));
	state->seed = *(cmph_uint32 *)buf;
	state->hashfunc = CMPH_HASH_JENKINS;
	DEBUGP("Loaded wyhash state with seed %u\n", state->seed);
	return state;
}


/** \fn void wyhash_state_pack(wyhash_state_t *state, void *wyhash_packed);
 *  \brief Support the ability to pack a wyhash function into a preallocated contiguous memory space pointed by wyhash_packed.
 *  \param state points to the wyhash function
 *  \param wyhash_packed pointer to the contiguous memory area used to store the wyhash function. The size of wyhash_packed must be at least wyhash_state_packed_size()
 */
void wyhash_state_pack(wyhash_state_t *state, void *wyhash_packed)
{
	if (state && wyhash_packed)
	{
		memcpy(wyhash_packed, &(state->seed), sizeof(cmph_uint32));
	}
}

/** \fn cmph_uint32 wyhash_state_packed_size(wyhash_state_t *state);
 *  \brief Return the amount of space needed to pack a wyhash function.
 *  \return the size of the packed function or zero for failures
 */
cmph_uint32 wyhash_state_packed_size(void)
{
	return sizeof(cmph_uint32);
}


/** \fn cmph_uint32 wyhash_hash_packed(void *wyhash_packed, const char *k, cmph_uint32 keylen);
 *  \param wyhash_packed is a pointer to a contiguous memory area
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \return an integer that represents a hash value of 32 bits.
 */
cmph_uint32 wyhash_hash_packed(void *wyhash_packed, const char *k, cmph_uint32 keylen)
{
	cmph_uint32 hashes[3];
	__wyhash_hash_vector(*((cmph_uint32 *)wyhash_packed), (const unsigned char*)k, keylen, hashes);
	return hashes[2];
}

/** \fn wyhash_hash_vector_packed(void *wyhash_packed, const char *k, cmph_uint32 keylen, cmph_uint32 * hashes);
 *  \param wyhash_packed is a pointer to a contiguous memory area
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \param hashes is a pointer to a memory large enough to fit three 32-bit integers.
 */
void wyhash_hash_vector_packed(void *wyhash_packed, const char *k, cmph_uint32 keylen, cmph_uint32 * hashes)
{
	__wyhash_hash_vector(*((cmph_uint32 *)wyhash_packed), (const unsigned char*)k, keylen, hashes);
}
