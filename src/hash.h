#ifndef __CMPH_HASH_H__
#define __CMPH_HASH_H__

#include "cmph_types.h"

typedef union __hash_state_t hash_state_t;

hash_state_t *hash_state_new(CMPH_HASH, uint32 hashsize);
uint32 hash(hash_state_t *state, const char *key, uint32 keylen);
void hash_state_dump(hash_state_t *state, char **buf, uint32 *buflen);
hash_state_t *hash_state_load(const char *buf, uint32 buflen);
void hash_state_destroy(hash_state_t *state);

#endif
