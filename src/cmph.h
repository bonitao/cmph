#ifndef __CMPH_H__
#define __CMPH_H__

#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "cmph_types.h"

typedef struct __mph_t mph_t;
typedef struct __mphf_t mphf_t;

typedef struct 
{
	void *data;
	uint32 nkeys;
	int (*read)(void *, char **, uint32 *);
	void (*dispose)(void *, char *, uint32);
	void (*rewind)(void *);
} key_source_t;

/** Hash generation API **/
mph_t *mph_new(MPH_ALGO algo, key_source_t *key_source);
void mph_set_hashfuncs(mph_t *mph, CMPH_HASH *hashfuncs);
void mph_set_verbosity(mph_t *mph, uint32 verbosity);
void mph_destroy(mph_t *mph);
mphf_t *mph_create(mph_t *mph);

/** Hash querying API **/
mphf_t *mphf_load(FILE *f);
int mphf_dump(mphf_t *mphf, FILE *f);
uint32 mphf_search(mphf_t *mphf, const char *key, uint32 keylen);
uint32 mphf_size(mphf_t *mphf);
void mphf_destroy(mphf_t *mphf);

#ifdef __cplusplus
}
#endif

#endif
