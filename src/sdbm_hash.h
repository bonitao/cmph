#ifndef __SDBM_HASH_H__
#define __SDBM_HASH_H__

#include "hash.h"

typedef struct __sdbm_state_t
{
	CMPH_HASH hashfunc;
} sdbm_state_t;

sdbm_state_t *sdbm_state_new();
uint32 sdbm_hash(sdbm_state_t *state, const char *k, uint32 keylen);
void sdbm_state_dump(sdbm_state_t *state, char **buf, uint32 *buflen);
sdbm_state_t *sdbm_state_load(const char *buf, uint32 buflen);
void sdbm_state_destroy(sdbm_state_t *state);

#endif
