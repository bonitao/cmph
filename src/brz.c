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

//#define DEBUG
#include "debug.h"

static int brz_before_gen_graphs(cmph_config_t *mph, cmph_uint32 * disksize, cmph_uint32 * diskoffset);
static void brz_gen_graphs(cmph_config_t *mph, cmph_uint32 * disksize, cmph_uint32 * diskoffset, FILE * graphs_fd);
static char ** brz_read_keys_vd(FILE * graphs_fd, cmph_uint8 nkeys);
static void brz_destroy_keys_vd(char ** keys_vd, cmph_uint8 nkeys);
static void brz_copy_partial_mphf(brz_config_data_t *brz, bmz_data_t * bmzf, cmph_uint32 index, cmph_io_adapter_t *source);

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
static cmph_uint8 brz_verify_mphf(cmph_t * mphf, cmph_io_adapter_t *source)
{
	cmph_uint8 * hashtable = NULL;
	cmph_uint32 i;
	hashtable = (cmph_uint8*)malloc(source->nkeys*sizeof(cmph_uint8));
	source->rewind(source->data);
	memset(hashtable, 0, source->nkeys);
	//check all keys
	for (i = 0; i < source->nkeys; ++i)
	{
		cmph_uint32 h;
		char *buf;
		cmph_uint32 buflen = 0;
		source->read(source->data, &buf, &buflen);		
		h = cmph_search(mphf, buf, buflen);
		if(hashtable[h])
		{
			fprintf(stderr, "collision: %u\n",h);
			return 0;
		}
		//assert(hashtable[h]==0);
		hashtable[h] = 1;
		source->dispose(source->data, buf, buflen);
	}
	free(hashtable);
	return 1;
}

static cmph_uint8 brz_verify_mphf1(hash_state_t *h1, hash_state_t *h2, cmph_uint8 * g, cmph_uint32 n, cmph_io_adapter_t *source)
{
	cmph_uint8 * hashtable = NULL;
	cmph_uint32 i;
	hashtable = (cmph_uint8*)calloc(source->nkeys, sizeof(cmph_uint8));
	source->rewind(source->data);
	//memset(hashtable, 0, source->nkeys);
	//check all keys
	for (i = 0; i < source->nkeys; ++i)
	{		
		cmph_uint32 h1_v;
		cmph_uint32 h2_v;
		cmph_uint32 h;
		char *buf;
		cmph_uint32 buflen = 0;
		source->read(source->data, &buf, &buflen);	
			
		h1_v = hash(h1, buf, buflen) % n;

		h2_v = hash(h2, buf, buflen) % n;

		if (h1_v == h2_v && ++h2_v >= n) h2_v = 0;
		
		h = ((cmph_uint32)g[h1_v] + (cmph_uint32)g[h2_v]) % source->nkeys;

		if(hashtable[h])
		{
			fprintf(stderr, "collision: %u\n",h);
			return 0;
		}
		//assert(hashtable[h]==0);
		hashtable[h] = 1;
		source->dispose(source->data, buf, buflen);

	}
	free(hashtable);
	return 1;
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
	brz->c = c;	
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

//	graphs_fd = fopen("/colecao/fbotelho/cmph.tmp", "wb");
	graphs_fd = fopen("cmph.tmp", "wb");
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
//	graphs_fd = fopen("/colecao/fbotelho/cmph.tmp", "rb");
	graphs_fd = fopen("cmph.tmp", "rb");
	// codigo do algoritmo... 
	brz->h1 = (hash_state_t **)malloc(sizeof(hash_state_t *)*brz->k);
	brz->h2 = (hash_state_t **)malloc(sizeof(hash_state_t *)*brz->k);
	brz->g  = (cmph_uint8 **)  malloc(sizeof(cmph_uint8 *)  *brz->k);
	DEBUGP("Generating mphf\n");
	for(i = 0; i < brz->k; i++)
	{
		cmph_uint32 j;
		bmz_data_t * bmzf = NULL;
		cmph_uint8 nkeys = brz->size[i];
		if (nkeys == 0) continue;
		keys_vd = brz_read_keys_vd(graphs_fd, nkeys);
		// Source of keys
		source = cmph_io_vector_adapter(keys_vd, (cmph_uint32)nkeys);
		config = cmph_config_new(source);
		cmph_config_set_algo(config, CMPH_BMZ);
		cmph_config_set_graphsize(config, c);
		mphf_tmp = cmph_new(config);
		bmzf = (bmz_data_t *)mphf_tmp->data;		
		//assert(brz_verify_mphf(mphf_tmp, source));
		brz_copy_partial_mphf(brz, bmzf, i, source); // implementar		
		cmph_config_destroy(config);
		brz_destroy_keys_vd(keys_vd, nkeys);		
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
	brzf->c = brz->c;
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
// 		if(h3 == 6) 
// 		{
// 			DEBUGP("key = %s\n", key);
// 			DEBUGP("keylen = %u\n", keylen + 1);
// 		}

		mph->key_source->dispose(mph->key_source->data, key, keylen);
		if (brz->size[h3] == 255) return 0;
		brz->size[h3] = brz->size[h3] + 1;
		disksize[h3] = disksize[h3] + keylen + 1;
// 		if(h3 == 6) 
// 		{
// 			DEBUGP("disksize[%u]=%u \n", h3, disksize[h3]);
// 		}

	}
//	DEBUGP("size:%u offset: %u\n", brz->size[0], brz->offset[0]);
	for (e = 1; e < brz->k; ++e)
	{
		brz->offset[e] = brz->size[e-1] + brz->offset[e-1];
		diskoffset[e]  = disksize[e-1]  +  diskoffset[e-1];
/*		DEBUGP("disksize[%u]=%u diskoffset[%u]: %u\n", e, disksize[e], e, diskoffset[e]);
		DEBUGP("size[%u]=%u offset[%u]: %u\n", e, brz->size[e], e, brz->offset[e]);*/
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
/*		if(h3 == 6) 
		{
			DEBUGP("key = %s\n", key);
			DEBUGP("keylen = %u\n", keylen + 1);
		}*/
		fseek(graphs_fd, diskoffset[h3], SEEK_SET);
		fwrite(key, sizeof(char), keylen + 1, graphs_fd);
/*		if(h3 == 6) 
		{
			DEBUGP("diskoffset[%u]=%u \n", h3, diskoffset[h3]);
		}*/
		diskoffset[h3] = diskoffset[h3] + keylen + 1;
		mph->key_source->dispose(mph->key_source->data, key, keylen);
	}
}

static char ** brz_read_keys_vd(FILE * graphs_fd, cmph_uint8 nkeys)
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
		free(buf);		
	}	
	return keys_vd;
}

static void brz_destroy_keys_vd(char ** keys_vd, cmph_uint8 nkeys)
{
	cmph_uint8 i;
	for(i = 0; i < nkeys; i++) free(keys_vd[i]);
}

static void brz_copy_partial_mphf(brz_config_data_t *brz, bmz_data_t * bmzf, cmph_uint32 index, cmph_io_adapter_t *source)
{
	cmph_uint32 i;
	cmph_uint32 n = ceil(brz->c * brz->size[index]);

	brz->g[index] = (cmph_uint8 *)calloc(n, sizeof(cmph_uint8));
	for(i = 0; i < n; i++)
	{
		brz->g[index][i] = (cmph_uint8) bmzf->g[i];
		//fprintf(stderr, "gsrc[%u]: %u  gdest: %u\n", i, (cmph_uint8) bmzf->g[i], brz->g[index][i]);
	}	
	brz->h1[index] = hash_state_copy(bmzf->hashes[0]);
	brz->h2[index] = hash_state_copy(bmzf->hashes[1]);
	//brz->size[index] = bmzf->n;
	//assert(brz_verify_mphf1(brz->h1[index], brz->h2[index], brz->g[index], n, source));
}

int brz_dump(cmph_t *mphf, FILE *fd)
{
	char *buf = NULL;
	cmph_uint32 buflen;
	cmph_uint32 nbuflen;
	cmph_uint32 i;
	brz_data_t *data = (brz_data_t *)mphf->data;
	DEBUGP("Dumping brzf\n");
	__cmph_dump(mphf, fd);

	fwrite(&(data->k), sizeof(cmph_uint32), 1, fd);
	//dumping h1 and h2.
	for(i = 0; i < data->k; i++)
	{
		// h1
		hash_state_dump(data->h1[i], &buf, &buflen);	
		DEBUGP("Dumping hash state with %u bytes to disk\n", buflen);
		fwrite(&buflen, sizeof(cmph_uint32), 1, fd);
		fwrite(buf, buflen, 1, fd);
		free(buf);
		// h2
		hash_state_dump(data->h2[i], &buf, &buflen);
		DEBUGP("Dumping hash state with %u bytes to disk\n", buflen);
		fwrite(&buflen, sizeof(cmph_uint32), 1, fd);
		fwrite(buf, buflen, 1, fd);
		free(buf);
	}
	// Dumping h3.
	hash_state_dump(data->h3, &buf, &buflen);
	DEBUGP("Dumping hash state with %u bytes to disk\n", buflen);
	fwrite(&buflen, sizeof(cmph_uint32), 1, fd);
	fwrite(buf, buflen, 1, fd);
	free(buf);
	
	// Dumping c, m, size vector and offset vector.
	fwrite(&(data->c), sizeof(cmph_float32), 1, fd);
	fwrite(&(data->m), sizeof(cmph_uint32), 1, fd);
	fwrite(data->size, sizeof(cmph_uint8)*(data->k), 1, fd);
	fwrite(data->offset, sizeof(cmph_uint32)*(data->k), 1, fd);
	
	// Dumping g function.
	for(i = 0; i < data->k; i++)
	{
		cmph_uint32 n = ceil(data->c * data->size[i]);
		fwrite(data->g[i], sizeof(cmph_uint8)*n, 1, fd);
	}
	return 1;
}

void brz_load(FILE *f, cmph_t *mphf)
{
	cmph_uint32 nhashes;
	char *buf = NULL;
	cmph_uint32 buflen;
	cmph_uint32 i;
	brz_data_t *brz = (brz_data_t *)malloc(sizeof(brz_data_t));

	DEBUGP("Loading brz mphf\n");
	mphf->data = brz;
	fread(&(brz->k), sizeof(cmph_uint32), 1, f);
	brz->h1 = (hash_state_t **)malloc(sizeof(hash_state_t *)*brz->k);
	brz->h2 = (hash_state_t **)malloc(sizeof(hash_state_t *)*brz->k);
	DEBUGP("Reading %u h1 and %u h2\n", brz->k, brz->k);
	//loading h1 and h2.
	for(i = 0; i < brz->k; i++)
	{
		// h1
		fread(&buflen, sizeof(cmph_uint32), 1, f);
		DEBUGP("Hash state has %u bytes\n", buflen);
		buf = (char *)malloc(buflen);
		fread(buf, buflen, 1, f);
		brz->h1[i] = hash_state_load(buf, buflen);
		free(buf);
		//h2
		fread(&buflen, sizeof(cmph_uint32), 1, f);
		DEBUGP("Hash state has %u bytes\n", buflen);
		buf = (char *)malloc(buflen);
		fread(buf, buflen, 1, f);
		brz->h2[i] = hash_state_load(buf, buflen);
		free(buf);		
	}
	//loading h3
	fread(&buflen, sizeof(cmph_uint32), 1, f);
	DEBUGP("Hash state has %u bytes\n", buflen);
	buf = (char *)malloc(buflen);
	fread(buf, buflen, 1, f);
	brz->h3 = hash_state_load(buf, buflen);
	free(buf);		

	//loading c, m, size vector and offset vector.
	fread(&(brz->c), sizeof(cmph_float32), 1, f);
	fread(&(brz->m), sizeof(cmph_uint32), 1, f);
	brz->size   = (cmph_uint8 *) malloc(sizeof(cmph_uint8)*brz->k);
	brz->offset = (cmph_uint32 *)malloc(sizeof(cmph_uint32)*brz->k);
	fread(brz->size, sizeof(cmph_uint8)*(brz->k), 1, f);
	fread(brz->offset, sizeof(cmph_uint32)*(brz->k), 1, f);
	
	//loading g function.
	brz->g  = (cmph_uint8 **)  malloc(sizeof(cmph_uint8 *)*brz->k);
	for(i = 0; i < brz->k; i++)
	{
		cmph_uint32 n = ceil(brz->c * brz->size[i]);
		brz->g[i] = (cmph_uint8 *)malloc(sizeof(cmph_uint8)*n);
		fread(brz->g[i], sizeof(cmph_uint8)*n, 1, f);
	}
	return;
}
		

cmph_uint32 brz_search(cmph_t *mphf, const char *key, cmph_uint32 keylen)
{
	brz_data_t *brz = mphf->data;	
	cmph_uint32 h3 = hash(brz->h3, key, keylen) % brz->k;
	cmph_uint32 m = brz->size[h3];
	cmph_uint32 n = ceil(brz->c * m);
	cmph_uint32 h1 = hash(brz->h1[h3], key, keylen) % n;
	cmph_uint32 h2 = hash(brz->h2[h3], key, keylen) % n;
	if (h1 == h2 && ++h2 >= n) h2 = 0;
	DEBUGP("key: %s h1: %u h2: %u h3: %u\n", key, h1, h2, h3);
	DEBUGP("key: %s g[h1]: %u g[h2]: %u offset[h3]: %u edges: %u\n", key, brz->g[h3][h1], brz->g[h3][h2], brz->offset[h3], brz->m);
	DEBUGP("Address: %u\n", (((cmph_uint32)brz->g[h3][h1] + (cmph_uint32)brz->g[h3][h2])% m + brz->offset[h3]));
	return (((cmph_uint32)brz->g[h3][h1] + (cmph_uint32)brz->g[h3][h2])% m + brz->offset[h3]);
}
void brz_destroy(cmph_t *mphf)
{
	cmph_uint32 i;
	brz_data_t *data = (brz_data_t *)mphf->data;
	for(i = 0; i < data->k; i++)
	{
		free(data->g[i]);
		hash_state_destroy(data->h1[i]);
		hash_state_destroy(data->h2[i]);
	}
	hash_state_destroy(data->h3);
	free(data->g);	
	free(data->h1);
	free(data->h2);
	free(data->size);
	free(data->offset);
	free(data);
	free(mphf);
}
