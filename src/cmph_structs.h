#ifndef __CMPH_STRUCTS_H__
#define __CMPH_STRUCTS_H__

#include "cmph.h"

/** Hash generation algorithm data
  */
struct cmph__mph_t
{
	CMPH_ALGO algo;
	cmph_key_source_t *key_source;
	cmph_uint32 verbosity;
	float c;
	void *data; //algorithm dependent data
};

/** Hash querying algorithm data
  */
struct cmph__mphf_t
{
	CMPH_ALGO algo;
	cmph_uint32 size;
	cmph_key_source_t *key_source;
	void *data; //algorithm dependent data
};

cmph_mph_t *cmph__mph_new(CMPH_ALGO algo, cmph_key_source_t *key_source);
void cmph__mph_destroy();
void cmph__mphf_dump(cmph_mphf_t *mphf, FILE *);
cmph_mphf_t *cmph__mphf_load(FILE *f);


#endif
