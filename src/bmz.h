#ifndef __CMPH_BMZ_H__
#define __CMPH_BMZ_H__

#include "cmph.h"

typedef struct __bmz_data_t bmz_data_t;
typedef struct __bmz_config_data_t bmz_config_data_t;

bmz_config_data_t *bmz_config_new(cmph_io_adapter_t *key_source);
void bmz_config_set_hashfuncs(cmph_config_t *mph, CMPH_HASH *hashfuncs);
void bmz_config_destroy(cmph_config_t *mph);
cmph_t *bmz_new(cmph_config_t *mph, float c);

void bmz_load(FILE *f, cmph_t *mphf);
int bmz_dump(cmph_t *mphf, FILE *f);
void bmz_destroy(cmph_t *mphf);
cmph_uint32 bmz_search(cmph_t *mphf, const char *key, cmph_uint32 keylen);
#endif
