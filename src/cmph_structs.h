#ifndef __CMPH_STRUCTS_H__
#define __CMPH_STRUCTS_H__

#include "cmph.h"

/** Hash generation algorithm data
  */
struct __config_t
{
	CMPH_ALGO algo;
	cmph_key_source_t *key_source;
	cmph_uint32 verbosity;
	float c;
	void *data; //algorithm dependent data
};

/** Hash querying algorithm data
  */
struct __cmph_t
{
	CMPH_ALGO algo;
	cmph_uint32 size;
	cmph_key_source_t *key_source;
	void *data; //algorithm dependent data
};

cmph_config_t *__config_new(cmph_key_source_t *key_source);
void __config_destroy();
void __cmph_dump(cmph_t *mphf, FILE *);
cmph_t *__cmph_load(FILE *f);


#endif
