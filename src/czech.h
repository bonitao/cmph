#ifndef __CMPH_CZECH_H__
#define __CMPH_CZECH_H__

#include "cmph.h"

typedef struct __czech_data_t czech_data_t;
typedef struct __czech_config_data_t czech_config_data_t;

czech_config_data_t *czech_config_new(cmph_io_adapter_t *key_source);
void czech_config_set_hashfuncs(cmph_config_t *mph, CMPH_HASH *hashfuncs);
void czech_config_destroy(cmph_config_t *mph);
cmph_t *czech_new(cmph_config_t *mph, float c);

void czech_load(FILE *f, cmph_t *mphf);
int czech_dump(cmph_t *mphf, FILE *f);
void czech_destroy(cmph_t *mphf);
cmph_uint32 czech_search(cmph_t *mphf, const char *key, cmph_uint32 keylen);
#endif
