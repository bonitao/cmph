#include "graph.h"
#include "bmz.h"
#include "bmz_structs.h"
#include "brz.h"
#include "cmph_structs.h"
#include "brz_structs.h"
#include "cmph.h"
#include "hash.h"
#include "bitbool.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#define DEBUG
#include "debug.h"

static int brz_before_gen_graphs(cmph_config_t *mph, cmph_uint32 * disksize, cmph_uint32 * diskoffset);
static void brz_gen_graphs(cmph_config_t *mph, cmph_uint32 * disksize, cmph_uint32 * diskoffset, FILE * graphs_fd);
static char ** brz_read_keys_vd(FILE * graphs_fd, cmph_uint8 nkeys, cmph_uint32 h3);
static void brz_destroy_keys_vd(char ** keys_vd, cmph_uint8 nkeys);
static void brz_copy_partial_mphf(brz_config_data_t *brz, bmz_data_t * bmzf, cmph_uint32 index);

brz_config_data_t *brz_config_new()
{
	brz_config_data_t *brz = NULL; 	
	brz = (brz_config_data_t *)malloc(sizeof(brz_config_data_t));
	brz->hashfuncs[0] = CMPH_HASH_JENKINS;
	brz->hashfuncs[1] = CMPH_HASH_JENKINS;
	brz->hashfuncs[2] = CMPH_HASH_JENKINS;
	brz->size   = NULL;
	brz->offset = NULL;
	brz->g      = NULL;
	brz->h1 = NULL;
	brz->h2 = NULL;
	brz->h3 = NULL;
	assert(brz);
	return brz;
}

void brz_config_destroy(cmph_config_t *mph)
{
	brz_config_data_t *data = (brz_config_data_t *)mph->data;
	DEBUGP("Destroying algorithm dependent data\n");
	free(data);
}

void brz_config_set_hashfuncs(cmph_config_t *mph, CMPH_HASH *hashfuncs)
{
	brz_config_data_t *brz = (brz_config_data_t *)mph->data;
	CMPH_HASH *hashptr = hashfuncs;
	cmph_uint32 i = 0;
	while(*hashptr != CMPH_HASH_COUNT)
	{
		if (i >= 3) break; //brz only uses three hash functions
		brz->hashfuncs[i] = *hashptr;	
		++i, ++hashptr;
	}
}

cmph_t *brz_new(cmph_config_t *mph, float c)
{
	cmph_t *mphf = NULL;
	brz_data_t *brzf = NULL;
	cmph_uint32 i;
	cmph_uint32 iterations = 20;
	cmph_uint32 * disksize = NULL;
	cmph_uint32 * diskoffset = NULL;
	
	cmph_io_adapter_t *source = NULL;
	cmph_config_t *config = NULL;
	cmph_t *mphf_tmp = NULL;
	char ** keys_vd = NULL;
	
	FILE * graphs_fd = NULL;
	DEBUGP("c: %f\n", c);
	brz_config_data_t *brz = (brz_config_data_t *)mph->data;
	brz->m = mph->key_source->nkeys;	
	DEBUGP("m: %u\n", brz->m);
	brz->k = ceil(brz->m/128);	
	DEBUGP("k: %u\n", brz->k);
	brz->size   = (cmph_uint8 *) malloc(sizeof(cmph_uint8)*brz->k);
	brz->offset = (cmph_uint32 *)malloc(sizeof(cmph_uint32)*brz->k);
	
	disksize   = (cmph_uint32 *)malloc(sizeof(cmph_uint32)*brz->k);
	diskoffset = (cmph_uint32 *)malloc(sizeof(cmph_uint32)*brz->k);
	
	for(i = 0; i < brz->k; ++i) 
	{
	  brz->size[i] = 0;
	  brz->offset[i] = 0;
	  disksize[i] = 0;
	  diskoffset[i] = 0;
	}
	
	// Creating the external graphs.
	while(1)
	{
		int ok;
		DEBUGP("hash function 3\n");
		brz->h3 = hash_state_new(brz->hashfuncs[2], brz->k);
		DEBUGP("Generating graphs\n");
		ok = brz_before_gen_graphs(mph, disksize, diskoffset);
		if (!ok)
		{
			--iterations;
			hash_state_destroy(brz->h3);
			brz->h3 = NULL;
			DEBUGP("%u iterations remaining to create the graphs in a external file\n", iterations);
			if (mph->verbosity)
			{
				fprintf(stderr, "Failure: A graph with more than 255 keys was created - %u iterations remaining\n", iterations);
			}
			if (iterations == 0) break;
		} 
		else break;	
	}
	
	if (iterations == 0) 
	{
		DEBUGP("Graphs with more than 255 keys were created in all 20 iterations\n");
		free(brz->size);
		free(brz->offset);
 		free(disksize);
		free(diskoffset);
		return NULL;
	}

	graphs_fd = fopen("/colecao/fbotelho/cmph.tmp", "wb");
	if (graphs_fd == NULL)
	{
		free(brz->size);
		free(brz->offset);
 		free(disksize);
		free(diskoffset);
		fprintf(stderr, "Unable to open file %s\n", "/colecao/fbotelho/cmph.tmp");
		return NULL;
	}
	// Clustering the keys by graph id.
	brz_gen_graphs(mph, disksize, diskoffset, graphs_fd);
	free(disksize);
	free(diskoffset);
	DEBUGP("Graphs generated\n");
	fclose(graphs_fd);
	graphs_fd = fopen("/colecao/fbotelho/cmph.tmp", "rb");
	// codigo do algoritmo... 
	brz->h1 = (hash_state_t **)malloc(sizeof(hash_state_t *)*brz->k);
	brz->h2 = (hash_state_t **)malloc(sizeof(hash_state_t *)*brz->k);
	brz->g  = (cmph_uint8 **)  malloc(sizeof(cmph_uint8 *)  *brz->k);
	
	for(i = 0; i < brz->k; i++)
	{
		cmph_uint32 j;
		bmz_data_t * bmzf = NULL;
		if (brz->size[i] == 0) continue;
		keys_vd = brz_read_keys_vd(graphs_fd, brz->size[i], i);
		// Source of keys
		source = cmph_io_vector_adapter(keys_vd, (cmph_uint32)brz->size[i]);
		config = cmph_config_new(source);
		cmph_config_set_algo(config, CMPH_BMZ);
		cmph_config_set_graphsize(config, c);
		mphf_tmp = cmph_new(config);
		bmzf = (bmz_data_t *)mphf_tmp->data;
		brz_copy_partial_mphf(brz, bmzf, i); // implementar		
		cmph_config_destroy(config);
		brz_destroy_keys_vd(keys_vd, brz->size[i]);		
		free(keys_vd);
		cmph_destroy(mphf_tmp);
		free(source);
	}
	
	fclose(graphs_fd);	
	
	// Generating a mphf
	mphf = (cmph_t *)malloc(sizeof(cmph_t));
	mphf->algo = mph->algo;
	brzf = (brz_data_t *)malloc(sizeof(brz_data_t));
	brzf->g = brz->g;
	brz->g = NULL; //transfer memory ownership
	brzf->h1 = brz->h1;
	brz->h1 = NULL; //transfer memory ownership
	brzf->h2 = brz->h2;
	brz->h2 = NULL; //transfer memory ownership
	brzf->h3 = brz->h3;
	brz->h3 = NULL; //transfer memory ownership
	brzf->size = brz->size;
	brz->size = NULL; //transfer memory ownership
	brzf->offset = brz->offset;
	brz->offset = NULL; //transfer memory ownership
	brzf->k = brz->k;
	brzf->m = brz->m;	
	mphf->data = brzf;
	mphf->size = brz->m;
	DEBUGP("Successfully generated minimal perfect hash\n");
	if (mph->verbosity)
	{
		fprintf(stderr, "Successfully generated minimal perfect hash function\n");
	}
	return mphf;
}

static int brz_before_gen_graphs(cmph_config_t *mph, cmph_uint32 * disksize, cmph_uint32 * diskoffset)
{
	cmph_uint32 e;
	brz_config_data_t *brz = (brz_config_data_t *)mph->data;
	mph->key_source->rewind(mph->key_source->data);
	DEBUGP("Generating information before the keys partition\n");
	for (e = 0; e < brz->m; ++e)
	{
		cmph_uint32 h3;
		cmph_uint32 keylen;
		char *key;
		mph->key_source->read(mph->key_source->data, &key, &keylen);
		h3 = hash(brz->h3, key, keylen) % brz->k;
		if(h3 == 6) 
		{
			DEBUGP("key = %s\n", key);
			DEBUGP("keylen = %u\n", keylen + 1);
		}

		mph->key_source->dispose(mph->key_source->data, key, keylen);
		if (brz->size[h3] == 255) return 0;
		brz->size[h3] = brz->size[h3] + 1;
		disksize[h3] = disksize[h3] + keylen + 1;
// 		if(h3 == 6) 
// 		{
// 			DEBUGP("disksize[%u]=%u \n", h3, disksize[h3]);
// 		}

	}
	fprintf(stderr, "size:%u offset: %u\n", brz->size[0], brz->offset[0]);
	for (e = 1; e < brz->k; ++e)
	{
		brz->offset[e] = brz->size[e-1] + brz->offset[e-1];
		diskoffset[e]  = disksize[e-1]  +  diskoffset[e-1];
		DEBUGP("disksize[%u]=%u diskoffset[%u]: %u\n", e, disksize[e], e, diskoffset[e]);
		DEBUGP("size[%u]=%u offset[%u]: %u\n", e, brz->size[e], e, brz->offset[e]);
	}
	return 1;
}

// disksize nao esta sendo usado ainda. Sera usado qd incluir os buffers.
static void brz_gen_graphs(cmph_config_t *mph, cmph_uint32 * disksize, cmph_uint32 * diskoffset, FILE * graphs_fd)
{
	cmph_uint32 e;
	brz_config_data_t *brz = (brz_config_data_t *)mph->data;
	mph->key_source->rewind(mph->key_source->data);
	DEBUGP("Generating graphs from %u keys\n", brz->m);
	for (e = 0; e < brz->m; ++e)
	{
		cmph_uint32 h3;
		cmph_uint32 keylen;
		char *key;
		mph->key_source->read(mph->key_source->data, &key, &keylen);
		h3 = hash(brz->h3, key, keylen) % brz->k;
		if(h3 == 6) 
		{
			DEBUGP("key = %s\n", key);
			DEBUGP("keylen = %u\n", keylen + 1);
		}
		fseek(graphs_fd, diskoffset[h3], SEEK_SET);
		fwrite(key, sizeof(char), keylen + 1, graphs_fd);
		if(h3 == 6) 
		{
			DEBUGP("diskoffset[%u]=%u \n", h3, diskoffset[h3]);
		}
		diskoffset[h3] = diskoffset[h3] + keylen + 1;
		mph->key_source->dispose(mph->key_source->data, key, keylen);
	}
}

static char ** brz_read_keys_vd(FILE * graphs_fd, cmph_uint8 nkeys, cmph_uint32 h3)
{
	char ** keys_vd = (char **)malloc(sizeof(char *)*nkeys);
	cmph_uint8 i;	
	
	for(i = 0; i < nkeys; i++)
	{
		char * buf = (char *)malloc(BUFSIZ);
		cmph_uint32 buf_pos = 0;
		char c;
		while(1)
		{
			
			fread(&c, sizeof(char), 1, graphs_fd);
			buf[buf_pos++] = c;
			if(c == '\0') break;
			if(buf_pos % BUFSIZ == 0) buf = (char *)realloc(buf, buf_pos + BUFSIZ);
		}		
		keys_vd[i] = (char *)malloc(strlen(buf) + 1);
		strcpy(keys_vd[i], buf);
		if(h3 == 6) DEBUGP("key = %s\n", keys_vd[i]);
		free(buf);		
	}	
	return keys_vd;
}

static void brz_destroy_keys_vd(char ** keys_vd, cmph_uint8 nkeys)
{
	cmph_uint8 i;
	for(i = 0; i < nkeys; i++) free(keys_vd[i]);
}

static void brz_copy_partial_mphf(brz_config_data_t *brz, bmz_data_t * bmzf, cmph_uint32 index)
{
	cmph_uint32 i;
	brz->g[index] = (cmph_uint8 *)malloc(sizeof(cmph_uint8)*bmzf->m);
	for(i = 0; i < bmzf->m; i++)
	{
		brz->g[index][i] = (cmph_uint8) bmzf->g[i];
	}	
	brz->h1[index] = bmzf->hashes[0];
	brz->h2[index] = bmzf->hashes[1];
}

int brz_dump(cmph_t *mphf, FILE *fd)
{
	/*char *buf = NULL;
	cmph_uint32 buflen;
	cmph_uint32 nbuflen;
	cmph_uint32 i;
	cmph_uint32 two = 2; //number of hash functions
	brz_data_t *data = (brz_data_t *)mphf->data;
	cmph_uint32 nn, nm;
	__cmph_dump(mphf, fd);

	fwrite(&two, sizeof(cmph_uint32), 1, fd);

	hash_state_dump(data->hashes[0], &buf, &buflen);
	DEBUGP("Dumping hash state with %u bytes to disk\n", buflen);
	fwrite(&buflen, sizeof(cmph_uint32), 1, fd);
	fwrite(buf, buflen, 1, fd);
	free(buf);

	hash_state_dump(data->hashes[1], &buf, &buflen);
	DEBUGP("Dumping hash state with %u bytes to disk\n", buflen);
	fwrite(&buflen, sizeof(cmph_uint32), 1, fd);
	fwrite(buf, buflen, 1, fd);
	free(buf);

	fwrite(&(data->n), sizeof(cmph_uint32), 1, fd);
	fwrite(&(data->m), sizeof(cmph_uint32), 1, fd);
	
	fwrite(data->g, sizeof(cmph_uint32)*(data->n), 1, fd);
	#ifdef DEBUG
	fprintf(stderr, "G: ");
	for (i = 0; i < data->n; ++i) fprintf(stderr, "%u ", data->g[i]);
	fprintf(stderr, "\n");
	#endif
*/	
	return 1;
}

void brz_load(FILE *f, cmph_t *mphf)
{
/*	cmph_uint32 nhashes;
	char *buf = NULL;
	cmph_uint32 buflen;
	cmph_uint32 i;
	brz_data_t *brz = (brz_data_t *)malloc(sizeof(brz_data_t));

	DEBUGP("Loading brz mphf\n");
	mphf->data = brz;
	fread(&nhashes, sizeof(cmph_uint32), 1, f);
	brz->hashes = (hash_state_t **)malloc(sizeof(hash_state_t *)*(nhashes + 1));
	brz->hashes[nhashes] = NULL;
	DEBUGP("Reading %u hashes\n", nhashes);
	for (i = 0; i < nhashes; ++i)
	{
		hash_state_t *state = NULL;
		fread(&buflen, sizeof(cmph_uint32), 1, f);
		DEBUGP("Hash state has %u bytes\n", buflen);
		buf = (char *)malloc(buflen);
		fread(buf, buflen, 1, f);
		state = hash_state_load(buf, buflen);
		brz->hashes[i] = state;
		free(buf);
	}

	DEBUGP("Reading m and n\n");
	fread(&(brz->n), sizeof(cmph_uint32), 1, f);	
	fread(&(brz->m), sizeof(cmph_uint32), 1, f);	

	brz->g = (cmph_uint32 *)malloc(sizeof(cmph_uint32)*brz->n);
	fread(brz->g, brz->n*sizeof(cmph_uint32), 1, f);
	#ifdef DEBUG
	fprintf(stderr, "G: ");
	for (i = 0; i < brz->n; ++i) fprintf(stderr, "%u ", brz->g[i]);
	fprintf(stderr, "\n");
	#endif
	return;
*/
}
		

cmph_uint32 brz_search(cmph_t *mphf, const char *key, cmph_uint32 keylen)
{
/*	brz_data_t *brz = mphf->data;
	cmph_uint32 h1 = hash(brz->hashes[0], key, keylen) % brz->n;
	cmph_uint32 h2 = hash(brz->hashes[1], key, keylen) % brz->n;
	DEBUGP("key: %s h1: %u h2: %u\n", key, h1, h2);
	if (h1 == h2 && ++h2 > brz->n) h2 = 0;
	DEBUGP("key: %s g[h1]: %u g[h2]: %u edges: %u\n", key, brz->g[h1], brz->g[h2], brz->m);
	return brz->g[h1] + brz->g[h2];
*/
	return 0;
}
void brz_destroy(cmph_t *mphf)
{
	cmph_uint32 i;
	brz_data_t *data = (brz_data_t *)mphf->data;
	fprintf(stderr, "MERDAAAAAA %u\n", data->h3->hashfunc);
	for(i = 0; i < data->k; i++)
	{
		free(data->g[i]);
		fprintf(stderr, "MERDAAAAAA1 %u\n", data->h3->hashfunc);
		hash_state_destroy(data->h1[i]);
		fprintf(stderr, "MERDAAAAAA2 %u\n", data->h3->hashfunc);
		hash_state_destroy(data->h2[i]);
		fprintf(stderr, "MERDAAAAAA3 %u\n", data->h3->hashfunc);
	}
	fprintf(stderr, "MERDAAAAAA %u\n", data->h3->hashfunc);
	hash_state_destroy(data->h3);
	fprintf(stderr, "MERDAAAAAA1FAB\n");
	free(data->g);	
	free(data->h1);
	free(data->h2);
	free(data->size);
	free(data->offset);
	free(data);
	free(mphf);
}
