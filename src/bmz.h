#ifndef __BMZ_H__
#define __BMZ_H__

#include "graph.h"
#include "cmph.h"

typedef struct __bmz_mphf_data_t bmz_mphf_data_t;
typedef struct __bmz_mph_data_t bmz_mph_data_t;

mph_t *bmz_mph_new(key_source_t *key_source);
void bmz_mph_set_hashfuncs(mph_t *mph, CMPH_HASH *hashfuncs);
void bmz_mph_destroy(mph_t *mph);
mphf_t *bmz_mph_create(mph_t *mph, float bmz_c);

void bmz_mphf_load(FILE *f, mphf_t *mphf);
int bmz_mphf_dump(mphf_t *mphf, FILE *f);
void bmz_mphf_destroy(mphf_t *mphf);
uint32 bmz_mphf_search(mphf_t *mphf, const char *key, uint32 keylen);
#endif
