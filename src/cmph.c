#include "cmph.h"
#include "cmph_structs.h"
#include "czech.h"
#include "bmz.h"
//#include "bmz.h" /* included -- Fabiano */

#include <stdlib.h>
#include <assert.h>

//#define DEBUG
#include "debug.h"

const char *cmph_names[] = { "bmz", "czech", NULL }; /* included -- Fabiano */

cmph_config_t *cmph_config_new(cmph_key_source_t *key_source)
{
	cmph_config_t *mph = NULL;
	mph = __config_new(key_source);
	assert(mph);
	mph->algo = CMPH_CZECH; // default value
	return mph;
}

void cmph_config_set_algo(cmph_config_t *mph, CMPH_ALGO algo)
{
        mph->algo = algo;
}

void cmph_config_destroy(cmph_config_t *mph)
{
	DEBUGP("Destroying mph with algo %s\n", cmph_names[mph->algo]);
	switch (mph->algo)
	{
		case CMPH_CZECH:
			czech_config_destroy(mph);
			break;
		case CMPH_BMZ: /* included -- Fabiano */
		        bmz_config_destroy(mph);
			break;
		default:
			assert(0);
	}
	__config_destroy(mph);
}

void cmph_config_set_verbosity(cmph_config_t *mph, cmph_uint32 verbosity)
{
	mph->verbosity = verbosity;
}

void cmph_config_set_hashfuncs(cmph_config_t *mph, CMPH_HASH *hashfuncs)
{
	switch (mph->algo)
	{
		case CMPH_CZECH:
			czech_config_set_hashfuncs(mph, hashfuncs);
			break;
		case CMPH_BMZ: /* included -- Fabiano */
			bmz_config_set_hashfuncs(mph, hashfuncs);
			break;
		default:
			break;
	}
	return;
}
void cmph_config_set_graphsize(cmph_config_t *mph, float c)
{
	mph->c = c;
	return;
}

cmph_t *cmph_new(cmph_config_t *mph)
{
	cmph_t *mphf = NULL;
	float c = mph->c;

	DEBUGP("Creating mph with algorithm %s\n", cmph_names[mph->algo]);
	switch (mph->algo)	
	{
		case CMPH_CZECH:
			DEBUGP("Creating czech hash\n");
			mph->data = czech_config_new(mph->key_source);
			if (c == 0) c = 2.09;
			mphf = czech_new(mph, c);
			break;
		case CMPH_BMZ: /* included -- Fabiano */
			DEBUGP("Creating bmz hash\n");
			mph->data = bmz_config_new(mph->key_source);
			if (c == 0) c = 1.15;
			mphf = bmz_new(mph, c);
			break;
		default:
			assert(0);
	}
	return mphf;
}

int cmph_dump(cmph_t *mphf, FILE *f)
{
	switch (mphf->algo)
	{
		case CMPH_CZECH:
			return czech_dump(mphf, f);
			break;
		case CMPH_BMZ: /* included -- Fabiano */
		        return bmz_dump(mphf, f);
			break;
		default:
			assert(0);
	}
	assert(0);
	return 0;
}
cmph_t *cmph_load(FILE *f)
{
	cmph_t *mphf = NULL;
	DEBUGP("Loading mphf generic parts\n");
	mphf =  __cmph_load(f);
	if (mphf == NULL) return NULL;
	DEBUGP("Loading mphf algorithm dependent parts\n");

	switch (mphf->algo)
	{
		case CMPH_CZECH:
			czech_load(f, mphf);
			break;
		case CMPH_BMZ: /* included -- Fabiano */
		        DEBUGP("Loading bmz algorithm dependent parts\n");
		        bmz_load(f, mphf);
			break;
		default:
			assert(0);
	}
	DEBUGP("Loaded mphf\n");
	return mphf;
}


cmph_uint32 cmph_search(cmph_t *mphf, const char *key, cmph_uint32 keylen)
{
      	DEBUGP("mphf algorithm: %u \n", mphf->algo);
	switch(mphf->algo)
	{
		case CMPH_CZECH:
			return czech_search(mphf, key, keylen);
		case CMPH_BMZ: /* included -- Fabiano */
		        DEBUGP("bmz algorithm search\n");		         
		        return bmz_search(mphf, key, keylen);
		default:
			assert(0);
	}
	assert(0);
	return 0;
}

cmph_uint32 cmph_size(cmph_t *mphf)
{
	return mphf->size;
}
	
void cmph_destroy(cmph_t *mphf)
{
	switch(mphf->algo)
	{
		case CMPH_CZECH:
			czech_destroy(mphf);
			return;
		case CMPH_BMZ: /* included -- Fabiano */
		        bmz_destroy(mphf);
			return;
		default: 
			assert(0);
	}
	assert(0);
	return;
}
