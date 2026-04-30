#include "config.h"
#include "crc32_hash.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <limits.h>

//#define DEBUG
#include "debug.h"

crc32_state_t *crc32_state_new(cmph_uint32 size)
{
	crc32_state_t *state = (crc32_state_t *)malloc(sizeof(crc32_state_t));
	if (!state) return NULL;
	if (size > 0) state->seed = ((cmph_uint32)rand() % size);
	else state->seed = 0;
#ifdef HAVE_CRC32_HW
	DEBUGP("Initializing HW crc32 hash with seed %u\n", state->seed);
#else
	DEBUGP("Initializing SW crc32 hash with seed %u\n", state->seed);
#endif
	return state;
}

void crc32_state_destroy(crc32_state_t *state)
{
	free(state);
}

#if !defined HAVE_CRC32_HW

/*
 * CRC-32C table for the SW calc, reflecting the HW intrinsic.
 * poly = 0x82f63b78U
 */
static const uint32_t crc32c_table[256] = {
    0x00000001, 0x493c7d27, 0xf20c0dfe, 0xba4fc28e, 0x3da6d0cb, 0xddc0152b, 0x1c291d04, 0x9e4addf8,
    0x740eef02, 0x39d3b296, 0x083a6eec, 0x0715ce53, 0xc49f4f67, 0x47db8317, 0x2ad91c30, 0x0d3b6092,
    0x6992cea2, 0xc96cfdc0, 0x7e908048, 0x878a92a7, 0x1b3d8f29, 0xdaece73e, 0xf1d0f55e, 0xab7aff2a,
    0xa87ab8a8, 0x2162d385, 0x8462d800, 0x83348832, 0x71d111a8, 0x299847d5, 0xffd852c6, 0xb9e02b86,
    0xdcb17aa4, 0x18b33a4e, 0xf37c5aee, 0xb6dd949b, 0x6051d5a2, 0x78d9ccb7, 0x18b0d4ff, 0xbac2fd7b,
    0x21f3d99c, 0xa60ce07b, 0x8f158014, 0xce7f39f4, 0xa00457f7, 0x61d82e56, 0x8d6d2c43, 0xd270f1a2,
    0x00ac29cf, 0xc619809d, 0xe9adf796, 0x2b3cac5d, 0x96638b34, 0x65863b64, 0xe0e9f351, 0x1b03397f,
    0x9af01f2d, 0xebb883bd, 0x2cff42cf, 0xb3e32c28, 0x88f25a3a, 0x064f7f26, 0x4e36f0b0, 0xdd7e3b0c,
    0xbd6f81f8, 0xf285651c, 0x91c9bd4b, 0x10746f3c, 0x885f087b, 0xc7a68855, 0x4c144932, 0x271d9844,
    0x52148f02, 0x8e766a0c, 0xa3c6f37a, 0x93a5f730, 0xd7c0557f, 0x6cb08e5c, 0x63ded06a, 0x6b749fb2,
    0x4d56973c, 0x1393e203, 0x9669c9df, 0xcec3662e, 0xe417f38a, 0x96c515bb, 0x4b9e0f71, 0xe6fc4e6a,
    0xd104b8fc, 0x8227bb8a, 0x5b397730, 0xb0cd4768, 0xe78eb416, 0x39c7ff35, 0x61ff0e01, 0xd7a4825c,
    0x8d96551c, 0x0ab3844b, 0x0bf80dd2, 0x0167d312, 0x8821abed, 0xf6076544, 0x6a45d2b2, 0x26f6a60a,
    0xd8d26619, 0xa741c1bf, 0xde87806c, 0x98d8d9cb, 0x14338754, 0x49c3cc9c, 0x5bd2011f, 0x68bce87a,
    0xdd07448e, 0x57a3d037, 0xdde8f5b9, 0x6956fc3b, 0xa3e3e02c, 0x42d98888, 0xd73c7bea, 0x3771e98f,
    0x80ff0093, 0xb42ae3d9, 0x8fe4c34d, 0x2178513a, 0xdf99fc11, 0xe0ac139e, 0x6c23e841, 0x170076fa
};
#if 0
// computed by
void compute_crc32c_table(uint32_t* crc32c_table, uint32_t n)
{
    uint64_t r = 1;
    for (uint32_t i = 0; i < n << 1; ++i)
    {
        crc32c_table[i] = (uint32_t)r;
        r = _mm_crc32_u64(r, 0);
    }
    printf("static const uint32_t crc32c_table[256] = {\n");
    for (uint32_t i = 0; i < n; ++i)
    {
        printf("0x%08x,%c", crc32c_table[i], (i & 7) == 7 ? '\n' : ' ');
    }
    printf("};\n");
}
#endif

static inline uint32_t
crc32c(uint32_t crc, uint8_t *data, size_t length)
{
	while (length--) {
		crc = crc32c_table[(crc ^ *data++) & 0xFF] ^ (crc >> 8);
	}
	return crc ^ 0xffffffff;
}

/* 3x 32bit hashes. */
static inline void
crc3(uint8_t *key, size_t len, uint32_t seed0, uint32_t seed1, uint32_t *hashes)
{
	hashes[0] = crc32c(seed0, key, len);
	hashes[1] = crc32c(seed1, key, len);
	hashes[2] = crc32c(seed0 ^ seed1, key, len);
}

#else // HAVE_CRC32_HW

#if defined(__aarch64__)
# include "sse2neon.h"
#else
# include <smmintrin.h>
#endif

#define ALIGN_SIZE 0x08UL
#define ALIGN_MASK (ALIGN_SIZE - 1)
#define CALC_CRC3(op, h1, h2, h3, type, buf, len)                  \
		for (; (len) >= sizeof(type);                      \
		     (len) -= sizeof(type), buf += sizeof(type)) { \
			(h1) = op((h1), *(type *)(buf));           \
			(h2) = op((h2), *(type *)(buf));           \
			(h3) = op((h3), *(type *)(buf));           \
		}                                                  \

static inline void
crc3(uint8_t *key, size_t len, uint32_t seed0, uint32_t seed1, uint32_t *hashes)
{
	uint32_t  h1 = seed0;
	uint32_t  h2 = seed1;
	uint32_t  h3 = h1 ^ h2;

	// Align the input to the word boundary
	for (; (len > 0) && ((size_t)key & ALIGN_MASK); len--, key++) {
		h1 = _mm_crc32_u8(h1, *key);
		h2 = _mm_crc32_u8(h2, *key);
		h3 = _mm_crc32_u8(h3, *key);
	}

#if defined(__x86_64__) || defined(__aarch64__)
	CALC_CRC3(_mm_crc32_u64, h1, h2, h3, uint64_t, key, len);
#endif
	CALC_CRC3(_mm_crc32_u32, h1, h2, h3, uint32_t, key, len);
	CALC_CRC3(_mm_crc32_u16, h1, h2, h3, uint16_t, key, len);
	CALC_CRC3(_mm_crc32_u8, h1, h2, h3, uint8_t, key, len);

	hashes[0] = h1;
	hashes[1] = h2;
	hashes[2] = h3;
}

static inline uint32_t
crc32c(uint32_t crc, uint8_t *key, size_t len)
{
	uint32_t h1 = crc;
	// Align the input to the word boundary
	for (; (len > 0) && ((size_t)key & ALIGN_MASK); len--, key++) {
		h1 = _mm_crc32_u8(h1, *key);
	}
#if defined(__x86_64__) || defined(__aarch64__)
        for (; len >= 8; len -= 8, key += 8) {
                h1 = _mm_crc32_u64(h1, *(uint64_t*)key);
        }
#else
        for (; len >= 4; len -= 4, key += 4) {
                h1 = _mm_crc32_u32(h1, *(uint32_t*)key);
        }
#endif
        for (; len >= 1; len--, key++) {
                h1 = _mm_crc32_u8(h1, *key);
        }
	return h1;
}

#endif

cmph_uint32 crc32_hash(crc32_state_t *state, const char *k, cmph_uint32 keylen)
{
	return crc32c(state->seed, (uint8_t *)k, keylen);
}

/* 3x 32bit hashes. */
void crc32_hash_vector_(crc32_state_t *state, const char *key, cmph_uint32 keylen, cmph_uint32 *hashes)
{
	crc3((uint8_t *)key, keylen, state->seed, state->seed + 1, hashes);
}

void crc32_state_dump(crc32_state_t *state, char **buf, cmph_uint32 *buflen)
{
	*buflen = sizeof(cmph_uint32);
	*buf = (char *)malloc(sizeof(cmph_uint32));
	if (!*buf)
	{
		*buflen = UINT_MAX;
		return;
	}
	memcpy(*buf, &(state->seed), sizeof(cmph_uint32));
#ifdef HAVE_CRC32_HW
	DEBUGP("Dumped HW crc32 state with seed %u\n", state->seed);
#else
	DEBUGP("Dumped SW crc32 state with seed %u\n", state->seed);
#endif
	return;
}

crc32_state_t *crc32_state_copy(crc32_state_t *src_state)
{
	crc32_state_t *dest_state = (crc32_state_t *)malloc(sizeof(crc32_state_t));
	dest_state->hashfunc = src_state->hashfunc;
	dest_state->seed = src_state->seed;
	return dest_state;
}

crc32_state_t *crc32_state_load(const char *buf, cmph_uint32 buflen)
{
	crc32_state_t *state = (crc32_state_t *)malloc(sizeof(crc32_state_t));
	state->seed = *(cmph_uint32 *)buf;
	state->hashfunc = CMPH_HASH_CRC32;
#ifdef HAVE_CRC32_HW
	DEBUGP("Loaded HW crc32 state with seed %u\n", state->seed);
#else
	DEBUGP("Loaded SW crc32 state with seed %u\n", state->seed);
#endif
	return state;
}

/** \fn void crc32_state_pack(crc32_state_t *state, void *crc32_packed);
 *  \brief Support the ability to pack a crc32 function into a preallocated contiguous memory space pointed by crc32_packed.
 *  \param state points to the crc32 function
 *  \param crc32_packed pointer to the contiguous memory area used to store the crc32 function. The size of crc32_packed must be at least crc32_state_packed_size()
 */
void crc32_state_pack(crc32_state_t *state, void *crc32_packed)
{
	if (state && crc32_packed)
	{
		memcpy(crc32_packed, &(state->seed), sizeof(cmph_uint32));
	}
}

/** \fn cmph_uint32 crc32_state_packed_size();
 *  \brief Return the amount of space needed to pack a crc32 function.
 *  \return the size of the packed function or zero for failures
 */
cmph_uint32 crc32_state_packed_size(void)
{
	return sizeof(cmph_uint32);
}


/** \fn cmph_uint32 crc32_hash_packed(void *crc32_packed, const char *k, cmph_uint32 keylen);
 *  \param crc32_packed is a pointer to a contiguous memory area
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \return an integer that represents a hash value of 32 bits.
 */
cmph_uint32 crc32_hash_packed(void *crc32_packed, const char *k, cmph_uint32 keylen)
{
	crc32_state_t state = { CMPH_HASH_CRC32, *(cmph_uint32 *)crc32_packed };
	return crc32_hash(&state, k, keylen);
}

/** \fn crc32_hash_vector_packed(void *crc32_packed, const char *k, cmph_uint32 keylen, cmph_uint32 * hashes);
 *  \param crc32_packed is a pointer to a contiguous memory area
 *  \param key is a pointer to a key
 *  \param keylen is the key length
 *  \param hashes is a pointer to a memory large enough to fit three 32-bit integers.
 */
void crc32_hash_vector_packed(void *crc32_packed, const char *k, cmph_uint32 keylen, cmph_uint32 * hashes)
{
	crc32_state_t state = { CMPH_HASH_CRC32, *(cmph_uint32 *)crc32_packed };
	crc32_hash_vector_(&state, k, keylen, hashes);
}
