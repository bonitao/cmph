#include "cmph_structs.h"

#include <string.h>

#define DEBUG
#include "debug.h"

mph_t *__mph_new(MPH_ALGO algo, key_source_t *key_source)
{
	mph_t *mph = (mph_t *)malloc(sizeof(mph_t));
	DEBUGP("Creating mph with algorithm %s\n", mph_names[algo]);
	if (mph == NULL) return NULL;
	mph->algo = algo;
	mph->key_source = key_source;
	mph->verbosity = 0;
	return mph;
}

void __mph_destroy(mph_t *mph)
{
	free(mph);
}

void __mphf_dump(mphf_t *mphf, FILE *fd)
{
	uint32 nsize = htonl(mphf->size);
	fwrite(mph_names[mphf->algo], strlen(mph_names[mphf->algo]) + 1, 1, fd);
	fwrite(&nsize, sizeof(mphf->size), 1, fd);
}
mphf_t *__mphf_load(FILE *f) 
{
	mphf_t *mphf = NULL;
	uint32 i;
	char algo_name[BUFSIZ];
	char *ptr = algo_name;
	MPH_ALGO algo = MPH_COUNT;

	DEBUGP("Loading mphf\n");
	while(1)
	{
		uint32 c = fread(ptr, 1, 1, f);
		if (c != 1) return NULL;
		if (*ptr == 0) break;
		++ptr;
	}
	for(i = 0; i < MPH_COUNT; ++i)
	{
		if (strcmp(algo_name, mph_names[i]) == 0)
		{
			algo = i;
		}
	}
	if (algo == MPH_COUNT) 
	{
		DEBUGP("Algorithm %s not found\n", algo_name);
		return NULL;
	}
	mphf = (mphf_t *)malloc(sizeof(mphf_t));
	mphf->algo = algo;
	fread(&(mphf->size), sizeof(mphf->size), 1, f);
	mphf->size = ntohl(mphf->size);
	mphf->data = NULL;
	DEBUGP("Algorithm is %s and mphf is sized %u\n", mph_names[algo],  mphf->size);

	return mphf;
}


