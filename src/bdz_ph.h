#ifndef __CMPH_BDZ_PH_H__
#define __CMPH_BDZ_PH_H__

#include "cmph.h"

typedef struct __bdz_ph_data_t bdz_ph_data_t;
typedef struct __bdz_ph_config_data_t bdz_ph_config_data_t;

bdz_ph_config_data_t *bdz_ph_config_new();
void bdz_ph_config_set_hashfuncs(cmph_config_t *mph, CMPH_HASH *hashfuncs);
void bdz_ph_config_destroy(cmph_config_t *mph);
cmph_t *bdz_ph_new(cmph_config_t *mph, float c);

void bdz_ph_load(FILE *f, cmph_t *mphf);
int bdz_ph_dump(cmph_t *mphf, FILE *f);
void bdz_ph_destroy(cmph_t *mphf);
cmph_uint32 bdz_ph_search(cmph_t *mphf, const char *key, cmph_uint32 keylen);
#endif
