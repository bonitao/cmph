#ifndef __CMPH_H__
#define __CMPH_H__

#include <stdlib.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

#include "cmph_types.h"

typedef struct cmph__mph_t cmph_mph_t;
typedef struct cmph__mphf_t cmph_mphf_t;

typedef struct 
{
	void *data;
	cmph_uint32 nkeys;
	int (*read)(void *, char **, cmph_uint32 *);
	void (*dispose)(void *, char *, cmph_uint32);
	void (*rewind)(void *);
} cmph_key_source_t;

/** Hash generation API **/
cmph_mph_t *cmph_mph_new(CMPH_ALGO algo, cmph_key_source_t *key_source);
void cmph_mph_set_hashfuncs(cmph_mph_t *mph, CMPH_HASH *hashfuncs);
void cmph_mph_set_verbosity(cmph_mph_t *mph, cmph_uint32 verbosity);
void cmph_mph_set_graphsize(cmph_mph_t *mph, float c);
void cmph_mph_destroy(cmph_mph_t *mph);
cmph_mphf_t *cmph_mph_create(cmph_mph_t *mph);

/** Hash querying API **/
cmph_mphf_t *cmph_mphf_load(FILE *f);
int cmph_mphf_dump(cmph_mphf_t *mphf, FILE *f);
cmph_uint32 cmph_mphf_search(cmph_mphf_t *mphf, const char *key, cmph_uint32 keylen);
cmph_uint32 cmph_mphf_size(cmph_mphf_t *mphf);
void cmph_mphf_destroy(cmph_mphf_t *mphf);

#ifdef __cplusplus
}
#endif

#endif
