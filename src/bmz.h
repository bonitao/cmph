#ifndef __BMZ_H__
#define __BMZ_H__

#include "graph.h"
#include "cmph.h"

typedef struct cmph__bmz_mphf_data_t cmph_bmz_mphf_data_t;
typedef struct cmph__bmz_mph_data_t cmph_bmz_mph_data_t;

cmph_mph_t *cmph_bmz_mph_new(cmph_key_source_t *key_source);
void cmph_bmz_mph_set_hashfuncs(cmph_mph_t *mph, CMPH_HASH *hashfuncs);
void cmph_bmz_mph_destroy(cmph_mph_t *mph);
cmph_mphf_t *cmph_bmz_mph_create(cmph_mph_t *mph, float bmz_c);

void cmph_bmz_mphf_load(FILE *f, cmph_mphf_t *mphf);
int cmph_bmz_mphf_dump(cmph_mphf_t *mphf, FILE *f);
void cmph_bmz_mphf_destroy(cmph_mphf_t *mphf);
cmph_uint32 cmph_bmz_mphf_search(cmph_mphf_t *mphf, const char *key, cmph_uint32 keylen);
#endif
