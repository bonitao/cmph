#ifndef __CMPH_FCH_H__
#define __CMPH_FCH_H__

#include "cmph.h"

typedef struct __fch_data_t fch_data_t;
typedef struct __fch_config_data_t fch_config_data_t;

/* Parameters calculation */
cmph_uint32 fch_calc_b(cmph_float32 c, cmph_uint32 m);
cmph_float32 fch_calc_p1(cmph_uint32 m);
cmph_float32 fch_calc_p2(cmph_uint32 b);
cmph_uint32 mixh10h11h12(cmph_uint32 b, cmph_float32 p1, cmph_float32 p2, cmph_uint32 initial_index);

fch_config_data_t *fch_config_new();
void fch_config_set_hashfuncs(cmph_config_t *mph, CMPH_HASH *hashfuncs);
void fch_config_destroy(cmph_config_t *mph);
cmph_t *fch_new(cmph_config_t *mph, float c);

void fch_load(FILE *f, cmph_t *mphf);
int fch_dump(cmph_t *mphf, FILE *f);
void fch_destroy(cmph_t *mphf);
cmph_uint32 fch_search(cmph_t *mphf, const char *key, cmph_uint32 keylen);
#endif
