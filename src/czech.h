#ifndef __CZECH_H__
#define __CZECH_H__

#include "graph.h"
#include "cmph.h"

typedef struct cmph__czech_mphf_data_t cmph_czech_mphf_data_t;
typedef struct cmph__czech_mph_data_t cmph_czech_mph_data_t;

cmph_mph_t *cmph_czech_mph_new(cmph_key_source_t *key_source);
void cmph_czech_mph_set_hashfuncs(cmph_mph_t *mph, CMPH_HASH *hashfuncs);
void cmph_czech_mph_destroy(cmph_mph_t *mph);
cmph_mphf_t *cmph_czech_mph_create(cmph_mph_t *mph, float c);

void cmph_czech_mphf_load(FILE *f, cmph_mphf_t *mphf);
int cmph_czech_mphf_dump(cmph_mphf_t *mphf, FILE *f);
void cmph_czech_mphf_destroy(cmph_mphf_t *mphf);
cmph_uint32 cmph_czech_mphf_search(cmph_mphf_t *mphf, const char *key, cmph_uint32 keylen);
#endif
