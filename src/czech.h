#ifndef __CZECH_H__
#define __CZECH_H__

#include "graph.h"
#include "cmph.h"

typedef struct __czech_mphf_data_t czech_mphf_data_t;
typedef struct __czech_mph_data_t czech_mph_data_t;

mph_t *czech_mph_new(key_source_t *key_source);
void czech_mph_set_hashfuncs(mph_t *mph, CMPH_HASH *hashfuncs);
void czech_mph_destroy(mph_t *mph);
mphf_t *czech_mph_create(mph_t *mph, float c);

void czech_mphf_load(FILE *f, mphf_t *mphf);
int czech_mphf_dump(mphf_t *mphf, FILE *f);
void czech_mphf_destroy(mphf_t *mphf);
uint32 czech_mphf_search(mphf_t *mphf, const char *key, uint32 keylen);
#endif
