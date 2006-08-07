#include "cmph.h"
#include "cmph_structs.h"
#include "chm.h"
#include "bmz.h"
#include "bmz8.h" /* included -- Fabiano */
#include "brz.h" /* included -- Fabiano */
#include "fch.h" /* included -- Fabiano */

#include <stdlib.h>
#include <assert.h>
#include <string.h>
//#define DEBUG
#include "debug.h"

const char *cmph_names[] = { "bmz", "bmz8", "chm", "brz",  "fch", NULL }; /* included -- Fabiano */

typedef struct 
{
	void *vector;
	cmph_uint32 position; // access position when data is a vector	
} cmph_vector_t;

static cmph_io_adapter_t *cmph_io_vector_new(void * vector, cmph_uint32 nkeys);
static void cmph_io_vector_destroy(cmph_io_adapter_t * key_source);

static int key_nlfile_read(void *data, char **key, cmph_uint32 *keylen)
{
	FILE *fd = (FILE *)data;
	*key = NULL;
	*keylen = 0;
	while(1)
	{
		char buf[BUFSIZ];
		char *c = fgets(buf, BUFSIZ, fd); 
		if (c == NULL) return -1;
		if (feof(fd)) return -1;
		*key = (char *)realloc(*key, *keylen + strlen(buf) + 1);
		memcpy(*key + *keylen, buf, strlen(buf));
		*keylen += (cmph_uint32)strlen(buf);
		if (buf[strlen(buf) - 1] != '\n') continue;
		break;
	}
	if ((*keylen) && (*key)[*keylen - 1] == '\n')
	{
		(*key)[(*keylen) - 1] = 0;
		--(*keylen);
	}
	return *keylen;
}

static int key_byte_vector_read(void *data, char **key, cmph_uint32 *keylen)
{
	cmph_vector_t *cmph_vector = (cmph_vector_t *)data;
	cmph_uint8 **keys_vd = (cmph_uint8 **)cmph_vector->vector;
	memcpy(keylen, keys_vd[cmph_vector->position], sizeof(*keylen));
	*key = (char *)malloc(*keylen);
	memcpy(*key, keys_vd[cmph_vector->position] + sizeof(*keylen), *keylen);
	cmph_vector->position = cmph_vector->position + 1;
	return *keylen;

}

static int key_vector_read(void *data, char **key, cmph_uint32 *keylen)
{
        cmph_vector_t *cmph_vector = (cmph_vector_t *)data;
        char **keys_vd = (char **)cmph_vector->vector;
        *keylen = strlen(keys_vd[cmph_vector->position]);
        *key = (char *)malloc(*keylen + 1);
        strcpy(*key, keys_vd[cmph_vector->position]);
        cmph_vector->position = cmph_vector->position + 1;
	return *keylen;

}


static void key_nlfile_dispose(void *data, char *key, cmph_uint32 keylen)
{
	free(key);
}

static void key_vector_dispose(void *data, char *key, cmph_uint32 keylen)
{
	free(key);
}

static void key_nlfile_rewind(void *data)
{
	FILE *fd = (FILE *)data;
	rewind(fd);
}

static void key_vector_rewind(void *data)
{
	cmph_vector_t *cmph_vector = (cmph_vector_t *)data;
	cmph_vector->position = 0;
}

static cmph_uint32 count_nlfile_keys(FILE *fd)
{
	cmph_uint32 count = 0;
	rewind(fd);
	while(1)
	{
		char buf[BUFSIZ];
		fgets(buf, BUFSIZ, fd); 
		if (feof(fd)) break;
		if (buf[strlen(buf) - 1] != '\n') continue;
		++count;
	}
	rewind(fd);
	return count;
}

cmph_io_adapter_t *cmph_io_nlfile_adapter(FILE * keys_fd)
{
  cmph_io_adapter_t * key_source = (cmph_io_adapter_t *)malloc(sizeof(cmph_io_adapter_t));
  assert(key_source);
  key_source->data = (void *)keys_fd;
  key_source->nkeys = count_nlfile_keys(keys_fd);
  key_source->read = key_nlfile_read;
  key_source->dispose = key_nlfile_dispose;
  key_source->rewind = key_nlfile_rewind;
  return key_source;
}

void cmph_io_nlfile_adapter_destroy(cmph_io_adapter_t * key_source)
{
	free(key_source);
}

cmph_io_adapter_t *cmph_io_nlnkfile_adapter(FILE * keys_fd, cmph_uint32 nkeys)
{
  cmph_io_adapter_t * key_source = (cmph_io_adapter_t *)malloc(sizeof(cmph_io_adapter_t));
  assert(key_source);
  key_source->data = (void *)keys_fd;
  key_source->nkeys = nkeys;
  key_source->read = key_nlfile_read;
  key_source->dispose = key_nlfile_dispose;
  key_source->rewind = key_nlfile_rewind;
  return key_source;
}

void cmph_io_nlnkfile_adapter_destroy(cmph_io_adapter_t * key_source)
{
	free(key_source);
}

static cmph_io_adapter_t *cmph_io_vector_new(void * vector, cmph_uint32 nkeys)
{
	cmph_io_adapter_t * key_source = (cmph_io_adapter_t *)malloc(sizeof(cmph_io_adapter_t));
	cmph_vector_t * cmph_vector = (cmph_vector_t *)malloc(sizeof(cmph_vector_t));
	assert(key_source);
	assert(cmph_vector);
	cmph_vector->vector = vector;
	cmph_vector->position = 0;
	key_source->data = (void *)cmph_vector;
	key_source->nkeys = nkeys;
	return key_source;
}

static void cmph_io_vector_destroy(cmph_io_adapter_t * key_source)
{
	cmph_vector_t *cmph_vector = (cmph_vector_t *)key_source->data;
	cmph_vector->vector = NULL;
	free(cmph_vector);
	free(key_source);
}

cmph_io_adapter_t *cmph_io_byte_vector_adapter(cmph_uint8 ** vector, cmph_uint32 nkeys)
{
	cmph_io_adapter_t * key_source = cmph_io_vector_new(vector, nkeys);
	key_source->read = key_byte_vector_read;
	key_source->dispose = key_vector_dispose;
	key_source->rewind = key_vector_rewind;
	return key_source;
}
void cmph_io_byte_vector_adapter_destroy(cmph_io_adapter_t * key_source)
{
	cmph_io_vector_destroy(key_source);
}
cmph_io_adapter_t *cmph_io_vector_adapter(char ** vector, cmph_uint32 nkeys)
{
	cmph_io_adapter_t * key_source = cmph_io_vector_new(vector, nkeys);
	key_source->read = key_vector_read;
	key_source->dispose = key_vector_dispose;
	key_source->rewind = key_vector_rewind;
	return key_source;
}

void cmph_io_vector_adapter_destroy(cmph_io_adapter_t * key_source)
{
	cmph_io_vector_destroy(key_source);
}

cmph_config_t *cmph_config_new(cmph_io_adapter_t *key_source)
{
	cmph_config_t *mph = NULL;
	mph = __config_new(key_source);
	assert(mph);
	mph->algo = CMPH_CHM; // default value
	mph->data = chm_config_new();
	return mph;
}

void cmph_config_set_algo(cmph_config_t *mph, CMPH_ALGO algo)
{
	if (algo != mph->algo)
	{
		switch (mph->algo)
		{
			case CMPH_CHM:
				chm_config_destroy(mph);
				break;
			case CMPH_BMZ:
				bmz_config_destroy(mph);
				break;
			case CMPH_BMZ8:
				bmz8_config_destroy(mph);
				break;
			case CMPH_BRZ:
				brz_config_destroy(mph);
				break;
			case CMPH_FCH:
				fch_config_destroy(mph);
				break;
			default:
				assert(0);
		}
		switch(algo)
		{
			case CMPH_CHM:
				mph->data = chm_config_new();
				break;
			case CMPH_BMZ:
				mph->data = bmz_config_new();
				break;
			case CMPH_BMZ8:
				mph->data = bmz8_config_new();
				break;
			case CMPH_BRZ:
				mph->data = brz_config_new();
				break;
			case CMPH_FCH:
				mph->data = fch_config_new();
				break;
			default:
				assert(0);
		}
	}
	mph->algo = algo;
}

void cmph_config_set_tmp_dir(cmph_config_t *mph, cmph_uint8 *tmp_dir)
{
	if (mph->algo == CMPH_BRZ) 
	{
		brz_config_set_tmp_dir(mph, tmp_dir);
	}
}


void cmph_config_set_mphf_fd(cmph_config_t *mph, FILE *mphf_fd)
{
	if (mph->algo == CMPH_BRZ) 
	{
		brz_config_set_mphf_fd(mph, mphf_fd);
	}
}

void cmph_config_set_b(cmph_config_t *mph, cmph_uint8 b)
{
	if (mph->algo == CMPH_BRZ) 
	{
		brz_config_set_b(mph, b);
	}
}

void cmph_config_set_memory_availability(cmph_config_t *mph, cmph_uint32 memory_availability)
{
	if (mph->algo == CMPH_BRZ) 
	{
		brz_config_set_memory_availability(mph, memory_availability);
	}
}

void cmph_config_destroy(cmph_config_t *mph)
{
	DEBUGP("Destroying mph with algo %s\n", cmph_names[mph->algo]);
	switch (mph->algo)
	{
		case CMPH_CHM:
			chm_config_destroy(mph);
			break;
		case CMPH_BMZ: /* included -- Fabiano */
			bmz_config_destroy(mph);
			break;
		case CMPH_BMZ8: /* included -- Fabiano */
	        	bmz8_config_destroy(mph);
			break;
		case CMPH_BRZ: /* included -- Fabiano */
	        	brz_config_destroy(mph);
			break;
		case CMPH_FCH: /* included -- Fabiano */
	        	fch_config_destroy(mph);
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
		case CMPH_CHM:
			chm_config_set_hashfuncs(mph, hashfuncs);
			break;
		case CMPH_BMZ: /* included -- Fabiano */
			bmz_config_set_hashfuncs(mph, hashfuncs);
			break;
		case CMPH_BMZ8: /* included -- Fabiano */
			bmz8_config_set_hashfuncs(mph, hashfuncs);
			break;
		case CMPH_BRZ: /* included -- Fabiano */
			brz_config_set_hashfuncs(mph, hashfuncs);
			break;
		case CMPH_FCH: /* included -- Fabiano */
			fch_config_set_hashfuncs(mph, hashfuncs);
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
		case CMPH_CHM:
			DEBUGP("Creating chm hash\n");
			mphf = chm_new(mph, c);
			break;
		case CMPH_BMZ: /* included -- Fabiano */
			DEBUGP("Creating bmz hash\n");
			mphf = bmz_new(mph, c);
			break;
		case CMPH_BMZ8: /* included -- Fabiano */
			DEBUGP("Creating bmz8 hash\n");
			mphf = bmz8_new(mph, c);
			break;
		case CMPH_BRZ: /* included -- Fabiano */
			DEBUGP("Creating brz hash\n");
			if (c >= 2.0) brz_config_set_algo(mph, CMPH_FCH);
			else brz_config_set_algo(mph, CMPH_BMZ8);
			mphf = brz_new(mph, c);
			break;
		case CMPH_FCH: /* included -- Fabiano */
			DEBUGP("Creating fch hash\n");
			mphf = fch_new(mph, c);
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
		case CMPH_CHM:
			return chm_dump(mphf, f);
		case CMPH_BMZ: /* included -- Fabiano */
		        return bmz_dump(mphf, f);
		case CMPH_BMZ8: /* included -- Fabiano */
		        return bmz8_dump(mphf, f);
		case CMPH_BRZ: /* included -- Fabiano */
		        return brz_dump(mphf, f);
		case CMPH_FCH: /* included -- Fabiano */
		        return fch_dump(mphf, f);
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
		case CMPH_CHM:
			chm_load(f, mphf);
			break;
		case CMPH_BMZ: /* included -- Fabiano */
			DEBUGP("Loading bmz algorithm dependent parts\n");
			bmz_load(f, mphf);
			break;
		case CMPH_BMZ8: /* included -- Fabiano */
			DEBUGP("Loading bmz8 algorithm dependent parts\n");
			bmz8_load(f, mphf);
			break;
		case CMPH_BRZ: /* included -- Fabiano */
			DEBUGP("Loading brz algorithm dependent parts\n");
			brz_load(f, mphf);
			break;
		case CMPH_FCH: /* included -- Fabiano */
			DEBUGP("Loading fch algorithm dependent parts\n");
			fch_load(f, mphf);
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
		case CMPH_CHM:
			return chm_search(mphf, key, keylen);
		case CMPH_BMZ: /* included -- Fabiano */
		        DEBUGP("bmz algorithm search\n");		         
		        return bmz_search(mphf, key, keylen);
		case CMPH_BMZ8: /* included -- Fabiano */
		        DEBUGP("bmz8 algorithm search\n");		         
		        return bmz8_search(mphf, key, keylen);
		case CMPH_BRZ: /* included -- Fabiano */
		        DEBUGP("brz algorithm search\n");		         
		        return brz_search(mphf, key, keylen);
		case CMPH_FCH: /* included -- Fabiano */
		        DEBUGP("fch algorithm search\n");		         
		        return fch_search(mphf, key, keylen);
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
		case CMPH_CHM:
			chm_destroy(mphf);
			return;
		case CMPH_BMZ: /* included -- Fabiano */
		        bmz_destroy(mphf);
			return;
		case CMPH_BMZ8: /* included -- Fabiano */
		        bmz8_destroy(mphf);
			return;
		case CMPH_BRZ: /* included -- Fabiano */
		        brz_destroy(mphf);
			return;
		case CMPH_FCH: /* included -- Fabiano */
		        fch_destroy(mphf);
			return;
		default: 
			assert(0);
	}
	assert(0);
	return;
}
