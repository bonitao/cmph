#include "cmph.h"
#include "cmph_structs.h"
#include "czech.h"
#include "bmz.h"
//#include "bmz.h" /* included -- Fabiano */

#include <stdlib.h>
#include <assert.h>

//#define DEBUG
#include "debug.h"

const char *mph_names[] = { "czech", "bmz", NULL }; /* included -- Fabiano */

mph_t *mph_new(MPH_ALGO algo, key_source_t *key_source)
{
	mph_t *mph = NULL;
	DEBUGP("Creating mph with algorithm %s\n", mph_names[algo]);
	switch (algo)
	{
		case MPH_CZECH:
			mph = czech_mph_new(key_source);
			break;
		case MPH_BMZ: /* included -- Fabiano */
		  	DEBUGP("new bmz algorithm \n");
		        mph = bmz_mph_new(key_source);
			break;
		default:
			assert(0);
	}
	assert(mph);
	return mph;
}

void mph_destroy(mph_t *mph)
{
	DEBUGP("Destroying mph with algo %s\n", mph_names[mph->algo]);
	switch (mph->algo)
	{
		case MPH_CZECH:
			czech_mph_destroy(mph);
			break;
		case MPH_BMZ: /* included -- Fabiano */
		        bmz_mph_destroy(mph);
			break;
		default:
			assert(0);
	}
}

void mph_set_verbosity(mph_t *mph, uint32 verbosity)
{
	mph->verbosity = verbosity;
}

void mph_set_hashfuncs(mph_t *mph, CMPH_HASH *hashfuncs)
{
	switch (mph->algo)
	{
		case MPH_CZECH:
			czech_mph_set_hashfuncs(mph, hashfuncs);
			break;
		case MPH_BMZ: /* included -- Fabiano */
			bmz_mph_set_hashfuncs(mph, hashfuncs);
			break;
		default:
			break;
	}
	return;
}
void mph_set_graphsize(mph_t *mph, float c)
{
	mph->c = c;
	return;
}

mphf_t *mph_create(mph_t *mph)
{
	mphf_t *mphf = NULL;
	float c = mph->c;
	switch (mph->algo)	
	{
		case MPH_CZECH:
			DEBUGP("Creating czech hash\n");
			if (c == 0) c = 2.09;
			mphf = czech_mph_create(mph, c);
			break;
		case MPH_BMZ: /* included -- Fabiano */
			DEBUGP("Creating bmz hash\n");
			if (c == 0) c = 1.15;
			mphf = bmz_mph_create(mph, c);
			break;
		default:
			assert(0);
	}
	return mphf;
}

int mphf_dump(mphf_t *mphf, FILE *f)
{
	switch (mphf->algo)
	{
		case MPH_CZECH:
			return czech_mphf_dump(mphf, f);
			break;
		case MPH_BMZ: /* included -- Fabiano */
		        return bmz_mphf_dump(mphf, f);
			break;
		default:
			assert(0);
	}
	assert(0);
	return 0;
}
mphf_t *mphf_load(FILE *f)
{
	mphf_t *mphf = NULL;
	DEBUGP("Loading mphf generic parts\n");
	mphf =  __mphf_load(f);
	if (mphf == NULL) return NULL;
	DEBUGP("Loading mphf algorithm dependent parts\n");

	switch (mphf->algo)
	{
		case MPH_CZECH:
			czech_mphf_load(f, mphf);
			break;
		case MPH_BMZ: /* included -- Fabiano */
		        DEBUGP("Loading bmz algorithm dependent parts\n");
		        bmz_mphf_load(f, mphf);
			break;
		default:
			assert(0);
	}
	DEBUGP("Loaded mphf\n");
	return mphf;
}


uint32 mphf_search(mphf_t *mphf, const char *key, uint32 keylen)
{
      	DEBUGP("mphf algorithm: %u \n", mphf->algo);
	switch(mphf->algo)
	{
		case MPH_CZECH:
			return czech_mphf_search(mphf, key, keylen);
		case MPH_BMZ: /* included -- Fabiano */
		        DEBUGP("bmz algorithm search\n");		         
		        return bmz_mphf_search(mphf, key, keylen);
		default:
			assert(0);
	}
	assert(0);
	return 0;
}

uint32 mphf_size(mphf_t *mphf)
{
	return mphf->size;
}
	
void mphf_destroy(mphf_t *mphf)
{
	switch(mphf->algo)
	{
		case MPH_CZECH:
			czech_mphf_destroy(mphf);
			return;
		case MPH_BMZ: /* included -- Fabiano */
		        bmz_mphf_destroy(mphf);
			return;
		default: 
			assert(0);
	}
	assert(0);
	return;
}
