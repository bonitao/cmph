#include "cmph_structs.h"

#include <string.h>

//#define DEBUG
#include "debug.h"

cmph_mph_t *cmph__mph_new(CMPH_ALGO algo, cmph_key_source_t *key_source)
{
	cmph_mph_t *mph = (cmph_mph_t *)malloc(sizeof(cmph_mph_t));
	DEBUGP("Creating mph with algorithm %s\n", cmph_names[algo]);
	if (mph == NULL) return NULL;
	mph->algo = algo;
	mph->key_source = key_source;
	mph->verbosity = 0;
	float c = 0;
	return mph;
}

void cmph__mph_destroy(cmph_mph_t *mph)
{
	free(mph);
}

void cmph__mphf_dump(cmph_mphf_t *mphf, FILE *fd)
{
	cmph_uint32 nsize = htonl(mphf->size);
	fwrite(cmph_names[mphf->algo], (cmph_uint32)(strlen(cmph_names[mphf->algo]) + 1), 1, fd);
	fwrite(&nsize, sizeof(mphf->size), 1, fd);
}
cmph_mphf_t *cmph__mphf_load(FILE *f) 
{
	cmph_mphf_t *mphf = NULL;
	cmph_uint32 i;
	char algo_name[BUFSIZ];
	char *ptr = algo_name;
	CMPH_ALGO algo = CMPH_COUNT;

	DEBUGP("Loading mphf\n");
	while(1)
	{
		cmph_uint32 c = fread(ptr, 1, 1, f);
		if (c != 1) return NULL;
		if (*ptr == 0) break;
		++ptr;
	}
	for(i = 0; i < CMPH_COUNT; ++i)
	{
		if (strcmp(algo_name, cmph_names[i]) == 0)
		{
			algo = i;
		}
	}
	if (algo == CMPH_COUNT) 
	{
		DEBUGP("Algorithm %s not found\n", algo_name);
		return NULL;
	}
	mphf = (cmph_mphf_t *)malloc(sizeof(cmph_mphf_t));
	mphf->algo = algo;
	fread(&(mphf->size), sizeof(mphf->size), 1, f);
	mphf->size = ntohl(mphf->size);
	mphf->data = NULL;
	DEBUGP("Algorithm is %s and mphf is sized %u\n", cmph_names[algo],  mphf->size);

	return mphf;
}


