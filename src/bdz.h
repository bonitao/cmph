#ifndef __CMPH_BDZ_H__
#define __CMPH_BDZ_H__

#include "cmph.h"

typedef struct __bdz_data_t bdz_data_t;
typedef struct __bdz_config_data_t bdz_config_data_t;

bdz_config_data_t *bdz_config_new();
void bdz_config_set_hashfuncs(cmph_config_t *mph, CMPH_HASH *hashfuncs);
void bdz_config_destroy(cmph_config_t *mph);
void bdz_config_set_b(cmph_config_t *mph, cmph_uint8 b);
cmph_t *bdz_new(cmph_config_t *mph, float c);

void bdz_load(FILE *f, cmph_t *mphf);
int bdz_dump(cmph_t *mphf, FILE *f);
void bdz_destroy(cmph_t *mphf);
cmph_uint32 bdz_search(cmph_t *mphf, const char *key, cmph_uint32 keylen);
#endif
