#include "cmph.h"
#include "cmph_structs.h"
#include "czech.h"
#include "bmz.h"
//#include "bmz.h" /* included -- Fabiano */

#include <stdlib.h>
#include <assert.h>

//#define DEBUG
#include "debug.h"

const char *cmph_names[] = { "czech", "bmz", NULL }; /* included -- Fabiano */

cmph_mph_t *cmph_mph_new(CMPH_ALGO algo, cmph_key_source_t *key_source)
{
	cmph_mph_t *mph = NULL;
	DEBUGP("Creating mph with algorithm %s\n", cmph_names[algo]);
	switch (algo)
	{
		case CMPH_CZECH:
			mph = cmph_czech_mph_new(key_source);
			break;
		case CMPH_BMZ: /* included -- Fabiano */
		  	DEBUGP("new bmz algorithm \n");
		        mph = cmph_bmz_mph_new(key_source);
			break;
		default:
			assert(0);
	}
	assert(mph);
	return mph;
}

void cmph_mph_destroy(cmph_mph_t *mph)
{
	DEBUGP("Destroying mph with algo %s\n", cmph_names[mph->algo]);
	switch (mph->algo)
	{
		case CMPH_CZECH:
			cmph_czech_mph_destroy(mph);
			break;
		case CMPH_BMZ: /* included -- Fabiano */
		        cmph_bmz_mph_destroy(mph);
			break;
		default:
			assert(0);
	}
}

void cmph_mph_set_verbosity(cmph_mph_t *mph, cmph_uint32 verbosity)
{
	mph->verbosity = verbosity;
}

void cmph_mph_set_hashfuncs(cmph_mph_t *mph, CMPH_HASH *hashfuncs)
{
	switch (mph->algo)
	{
		case CMPH_CZECH:
			cmph_czech_mph_set_hashfuncs(mph, hashfuncs);
			break;
		case CMPH_BMZ: /* included -- Fabiano */
			cmph_bmz_mph_set_hashfuncs(mph, hashfuncs);
			break;
		default:
			break;
	}
	return;
}
void cmph_mph_set_graphsize(cmph_mph_t *mph, float c)
{
	mph->c = c;
	return;
}

cmph_mphf_t *cmph_mph_create(cmph_mph_t *mph)
{
	cmph_mphf_t *mphf = NULL;
	float c = mph->c;
	switch (mph->algo)	
	{
		case CMPH_CZECH:
			DEBUGP("Creating czech hash\n");
			if (c == 0) c = 2.09;
			mphf = cmph_czech_mph_create(mph, c);
			break;
		case CMPH_BMZ: /* included -- Fabiano */
			DEBUGP("Creating bmz hash\n");
			if (c == 0) c = 1.15;
			mphf = cmph_bmz_mph_create(mph, c);
			break;
		default:
			assert(0);
	}
	return mphf;
}

int cmph_mphf_dump(cmph_mphf_t *mphf, FILE *f)
{
	switch (mphf->algo)
	{
		case CMPH_CZECH:
			return cmph_czech_mphf_dump(mphf, f);
			break;
		case CMPH_BMZ: /* included -- Fabiano */
		        return cmph_bmz_mphf_dump(mphf, f);
			break;
		default:
			assert(0);
	}
	assert(0);
	return 0;
}
cmph_mphf_t *cmph_mphf_load(FILE *f)
{
	cmph_mphf_t *mphf = NULL;
	DEBUGP("Loading mphf generic parts\n");
	mphf =  cmph__mphf_load(f);
	if (mphf == NULL) return NULL;
	DEBUGP("Loading mphf algorithm dependent parts\n");

	switch (mphf->algo)
	{
		case CMPH_CZECH:
			cmph_czech_mphf_load(f, mphf);
			break;
		case CMPH_BMZ: /* included -- Fabiano */
		        DEBUGP("Loading bmz algorithm dependent parts\n");
		        cmph_bmz_mphf_load(f, mphf);
			break;
		default:
			assert(0);
	}
	DEBUGP("Loaded mphf\n");
	return mphf;
}


cmph_uint32 cmph_mphf_search(cmph_mphf_t *mphf, const char *key, cmph_uint32 keylen)
{
      	DEBUGP("mphf algorithm: %u \n", mphf->algo);
	switch(mphf->algo)
	{
		case CMPH_CZECH:
			return cmph_czech_mphf_search(mphf, key, keylen);
		case CMPH_BMZ: /* included -- Fabiano */
		        DEBUGP("bmz algorithm search\n");		         
		        return cmph_bmz_mphf_search(mphf, key, keylen);
		default:
			assert(0);
	}
	assert(0);
	return 0;
}

cmph_uint32 cmph_mphf_size(cmph_mphf_t *mphf)
{
	return mphf->size;
}
	
void cmph_mphf_destroy(cmph_mphf_t *mphf)
{
	switch(mphf->algo)
	{
		case CMPH_CZECH:
			cmph_czech_mphf_destroy(mphf);
			return;
		case CMPH_BMZ: /* included -- Fabiano */
		        cmph_bmz_mphf_destroy(mphf);
			return;
		default: 
			assert(0);
	}
	assert(0);
	return;
}
