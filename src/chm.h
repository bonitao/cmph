#ifndef __CMPH_CHM_H__
#define __CMPH_CHM_H__

#include "cmph.h"

typedef struct __chm_data_t chm_data_t;
typedef struct __chm_config_data_t chm_config_data_t;

chm_config_data_t *chm_config_new();
void chm_config_set_hashfuncs(cmph_config_t *mph, CMPH_HASH *hashfuncs);
void chm_config_destroy(cmph_config_t *mph);
cmph_t *chm_new(cmph_config_t *mph, float c);

void chm_load(FILE *f, cmph_t *mphf);
int chm_dump(cmph_t *mphf, FILE *f);
void chm_destroy(cmph_t *mphf);
cmph_uint32 chm_search(cmph_t *mphf, const char *key, cmph_uint32 keylen);
#endif
