#include "cmph.h"
#include "cmph_structs.h"
#include "chm.h"
#include "bmz.h"
//#include "bmz.h" /* included -- Fabiano */

#include <stdlib.h>
#include <assert.h>

//#define DEBUG
#include "debug.h"

const char *cmph_names[] = { "bmz", "chm", NULL }; /* included -- Fabiano */

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

static void key_nlfile_dispose(void *data, char *key, cmph_uint32 keylen)
{
	free(key);
}
static void key_nlfile_rewind(void *data)
{
	FILE *fd = (FILE *)data;
	rewind(fd);
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
  cmph_io_adapter_t * key_source = malloc(sizeof(cmph_io_adapter_t));
  assert(key_source);
  key_source->data = (void *)keys_fd;
  key_source->nkeys = count_nlfile_keys(keys_fd);
  key_source->read = key_nlfile_read;
  key_source->dispose = key_nlfile_dispose;
  key_source->rewind = key_nlfile_rewind;
  return key_source;
}

cmph_io_adapter_t *cmph_io_nlnkfile_adapter(FILE * keys_fd, cmph_uint32 nkeys)
{
  cmph_io_adapter_t * key_source = malloc(sizeof(cmph_io_adapter_t));
  assert(key_source);
  key_source->data = (void *)keys_fd;
  key_source->nkeys = nkeys;
  key_source->read = key_nlfile_read;
  key_source->dispose = key_nlfile_dispose;
  key_source->rewind = key_nlfile_rewind;
  return key_source;
}

cmph_io_adapter_t *cmph_io_vector_adapter(const char ** vector, cmph_uint32 nkeys)
{
  return NULL;
}

cmph_config_t *cmph_config_new(cmph_io_adapter_t *key_source)
{
	cmph_config_t *mph = NULL;
	mph = __config_new(key_source);
	assert(mph);
	mph->algo = CMPH_CHM; // default value
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
		case CMPH_CHM:
			chm_config_destroy(mph);
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
		case CMPH_CHM:
			chm_config_set_hashfuncs(mph, hashfuncs);
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
		case CMPH_CHM:
			DEBUGP("Creating chm hash\n");
			mph->data = chm_config_new(mph->key_source);
			if (c == 0) c = 2.09;
			mphf = chm_new(mph, c);
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
		case CMPH_CHM:
			return chm_dump(mphf, f);
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
		case CMPH_CHM:
			chm_load(f, mphf);
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
		case CMPH_CHM:
			return chm_search(mphf, key, keylen);
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
		case CMPH_CHM:
			chm_destroy(mphf);
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
