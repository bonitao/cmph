#ifndef __CMPH_STRUCTS_H__
#define __CMPH_STRUCTS_H__

#include "cmph.h"

/** Hash generation algorithm data
  */
struct __mph_t
{
	MPH_ALGO algo;
	key_source_t *key_source;
	uint32 verbosity;
	float c;
	void *data; //algorithm dependent data
};

/** Hash querying algorithm data
  */
struct __mphf_t
{
	MPH_ALGO algo;
	uint32 size;
	key_source_t *key_source;
	void *data; //algorithm dependent data
};

mph_t *__mph_new(MPH_ALGO algo, key_source_t *key_source);
void __mph_destroy();
void __mphf_dump(mphf_t *mphf, FILE *);
mphf_t *__mphf_load(FILE *f);


#endif
