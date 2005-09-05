#ifndef __CMPH_BMZ8_H__
#define __CMPH_BMZ8_H__

#include "cmph.h"

typedef struct __bmz8_data_t bmz8_data_t;
typedef struct __bmz8_config_data_t bmz8_config_data_t;

bmz8_config_data_t *bmz8_config_new();
void bmz8_config_set_hashfuncs(cmph_config_t *mph, CMPH_HASH *hashfuncs);
void bmz8_config_destroy(cmph_config_t *mph);
cmph_t *bmz8_new(cmph_config_t *mph, float c);

void bmz8_load(FILE *f, cmph_t *mphf);
int bmz8_dump(cmph_t *mphf, FILE *f);
void bmz8_destroy(cmph_t *mphf);
cmph_uint8 bmz8_search(cmph_t *mphf, const char *key, cmph_uint32 keylen);
#endif
