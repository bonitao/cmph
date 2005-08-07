
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
#define MAX_BUCKET_SIZE 255
//#define DEBUG
#include "debug.h"

static int brz_gen_graphs(cmph_config_t *mph);
static cmph_uint32 brz_min_index(cmph_uint32 * vector, cmph_uint32 n);
static void flush_buffer(cmph_uint8 *buffer, cmph_uint32 *memory_usage, FILE * graphs_fd);
static void save_in_disk(cmph_uint8 *buffer, cmph_uint8 * key, cmph_uint32 keylen, cmph_uint32 *memory_usage, cmph_uint32 memory_availability, FILE * graphs_fd);
static char * brz_read_key(FILE * fd);
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
	fprintf(stderr, "\n===============================================================================\n");
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

	DEBUGP("c: %f\n");
	brz_config_data_t *brz = (brz_config_data_t *)mph->data;
	brz->c = c;
	brz->m = mph->key_source->nkeys;
	DEBUGP("m: %u\n", brz->m);
	brz->k = ceil(brz->m/170);
	DEBUGP("k: %u\n", brz->k);
	brz->size   = (cmph_uint8 *) calloc(brz->k, sizeof(cmph_uint8));
	
	// Clustering the keys by graph id.
	if (mph->verbosity)
	{
		fprintf(stderr, "Partioning the set of keys.\n");	
	}
	
	brz->h1 = (hash_state_t **)malloc(sizeof(hash_state_t *)*brz->k);
	brz->h2 = (hash_state_t **)malloc(sizeof(hash_state_t *)*brz->k);
	brz->g  = (cmph_uint8 **)  malloc(sizeof(cmph_uint8 *)  *brz->k);
	
	while(1)
	{
		int ok;
		DEBUGP("hash function 3\n");
		brz->h3 = hash_state_new(brz->hashfuncs[2], brz->k);
		DEBUGP("Generating graphs\n");
		ok = brz_gen_graphs(mph);
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
		return NULL;
	}
	DEBUGP("Graphs generated\n");
	
	brz->offset = (cmph_uint32 *)calloc(brz->k, sizeof(cmph_uint32));
	for (i = 1; i < brz->k; ++i)
	{
		brz->offset[i] = brz->size[i-1] + brz->offset[i-1];
	}

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

static int brz_gen_graphs(cmph_config_t *mph)
{
#pragma pack(1)
	cmph_uint32 i, e;
	brz_config_data_t *brz = (brz_config_data_t *)mph->data;
	cmph_uint32 memory_availability = 10485760; //10MB 209715200;//200MB //104857600;//100MB  //524288000; // 500MB //209715200; // 200 MB
	cmph_uint32 memory_usage = 0;
	cmph_uint32 nkeys_in_buffer = 0;
	cmph_uint8 *buffer = (cmph_uint8 *)malloc(memory_availability);
	cmph_uint32 *buckets_size = (cmph_uint32 *)calloc(brz->k, sizeof(cmph_uint32));	
	cmph_uint32 *keys_index = NULL;
	cmph_uint8 **buffer_merge = NULL;
	cmph_uint32 *buffer_h3 = NULL;
	cmph_uint32 nflushes = 0;
	cmph_uint32 h3;
	FILE *  tmp_fd = NULL;
	FILE ** tmp_fds = NULL;
	char filename[100];
	char *key = NULL;
	cmph_uint32 keylen;
	
	cmph_uint32 cur_bucket = 0;
	cmph_uint8 nkeys_vd = 0;
	char ** keys_vd = NULL;
	
	
	mph->key_source->rewind(mph->key_source->data);
	DEBUGP("Generating graphs from %u keys\n", brz->m);
	// Partitioning
	for (e = 0; e < brz->m; ++e)
	{
		mph->key_source->read(mph->key_source->data, &key, &keylen);
			
		/* Buffers management */
		if (memory_usage + keylen + 1 > memory_availability) // flush buffers 
		{ 
			if(mph->verbosity)
			{
				fprintf(stderr, "Flushing  %u\n", nkeys_in_buffer);
			}
			cmph_uint32 value = buckets_size[0];
			cmph_uint32 sum = 0;

			cmph_uint32 keylen1 = 0;			
			buckets_size[0]   = 0;			
			for(i = 1; i < brz->k; i++)
			{
				if(buckets_size[i] == 0) continue;
				sum += value;
				value = buckets_size[i];
				buckets_size[i] = sum;
				
			}	
			memory_usage = 0;
			keys_index = (cmph_uint32 *)calloc(nkeys_in_buffer, sizeof(cmph_uint32));
			for(i = 0; i < nkeys_in_buffer; i++)
			{
				keylen1 = strlen(buffer + memory_usage);
				h3 = hash(brz->h3, buffer + memory_usage, keylen1) % brz->k;
				keys_index[buckets_size[h3]] = memory_usage;
				buckets_size[h3]++;
				memory_usage = memory_usage + keylen1 + 1;
			}
			sprintf(filename, "/mnt/hd4/fbotelho/%u.cmph",nflushes);
/*			sprintf(filename, "%u.cmph",nflushes);*/
			tmp_fd = fopen(filename, "wb");
			for(i = 0; i < nkeys_in_buffer; i++)
			{
				keylen1 = strlen(buffer + keys_index[i]) + 1;
				fwrite(buffer + keys_index[i], 1, keylen1, tmp_fd);
			}
			nkeys_in_buffer = 0;
			memory_usage = 0;
			bzero(buckets_size, brz->k*sizeof(cmph_uint32));
			nflushes++;
			free(keys_index);
			fclose(tmp_fd);
		}
		//fprintf(stderr, "Storing read Key\n");
		memcpy(buffer + memory_usage, key, keylen + 1);
		memory_usage = memory_usage + keylen + 1;
		h3 = hash(brz->h3, key, keylen) % brz->k;
		if (brz->size[h3] == MAX_BUCKET_SIZE) 
		{
			free(buffer);
			free(buckets_size);
			return 0;
		}
		brz->size[h3] = brz->size[h3] + 1;
		buckets_size[h3] ++;
		nkeys_in_buffer++;

		mph->key_source->dispose(mph->key_source->data, key, keylen);
	}

	if (memory_usage != 0) // flush buffers 
	{ 
		if(mph->verbosity)
		{
			fprintf(stderr, "Flushing  %u\n", nkeys_in_buffer);
		}
		cmph_uint32 value = buckets_size[0];
		cmph_uint32 sum = 0;
		cmph_uint32 keylen1 = 0;
		buckets_size[0]   = 0;
		for(i = 1; i < brz->k; i++)
		{
			if(buckets_size[i] == 0) continue;
			sum += value;
			value = buckets_size[i];
			buckets_size[i] = sum;
		}
		memory_usage = 0;
		keys_index = (cmph_uint32 *)calloc(nkeys_in_buffer, sizeof(cmph_uint32));
		for(i = 0; i < nkeys_in_buffer; i++)
		{
			keylen1 = strlen(buffer + memory_usage);
			h3 = hash(brz->h3, buffer + memory_usage, keylen1) % brz->k;
			keys_index[buckets_size[h3]] = memory_usage;
			buckets_size[h3]++;
			memory_usage = memory_usage + keylen1 + 1;
		}
		sprintf(filename, "/mnt/hd4/fbotelho/%u.cmph",nflushes);
/*		sprintf(filename, "%u.cmph",nflushes);*/
		tmp_fd = fopen(filename, "wb");
		for(i = 0; i < nkeys_in_buffer; i++)
		{
			keylen1 = strlen(buffer + keys_index[i]) + 1;
			fwrite(buffer + keys_index[i], 1, keylen1, tmp_fd);
		}
		nkeys_in_buffer = 0;
		memory_usage = 0;
		bzero(buckets_size, brz->k*sizeof(cmph_uint32));
		nflushes++;
		free(keys_index);
		fclose(tmp_fd);
	}
	free(buffer);
	free(buckets_size);
	if(nflushes > 1024) return 0; // Too many files generated.
	
	// mphf generation
	if(mph->verbosity)
	{
		fprintf(stderr, "\nMPHF generation \n");
	}
	tmp_fds = (FILE **)calloc(nflushes, sizeof(FILE *));
	buffer_merge = (cmph_uint8 **)calloc(nflushes, sizeof(cmph_uint8 *));
	buffer_h3    = (cmph_uint32 *)calloc(nflushes, sizeof(cmph_uint32));
	
	for(i = 0; i < nflushes; i++)
	{
		sprintf(filename, "/mnt/hd4/fbotelho/%u.cmph",i);
/*		sprintf(filename, "%u.cmph",i);*/
		tmp_fds[i] = fopen(filename, "rb");
		key = brz_read_key(tmp_fds[i]);
		keylen = strlen(key);
		h3 = hash(brz->h3, key, keylen) % brz->k;
		buffer_h3[i] = h3;
		buffer_merge[i] = (cmph_uint8 *)calloc(keylen + 1, sizeof(cmph_uint8));
		memcpy(buffer_merge[i], key, keylen + 1);
		free(key);
	}
	
	e = 0;
	keys_vd = (char **)calloc(MAX_BUCKET_SIZE, sizeof(char *));
	nkeys_vd = 0;
	//buffer = (cmph_uint8 *)malloc(memory_availability);
	while(e < brz->m)
	{
		i = brz_min_index(buffer_h3, nflushes);
		cur_bucket = buffer_h3[i];
		key = brz_read_key(tmp_fds[i]);
		if(key)
		{
			while(key)
			{
				keylen = strlen(key);
				h3 = hash(brz->h3, key, keylen) % brz->k;
				
				if (h3 != buffer_h3[i]) break;
				
				keys_vd[nkeys_vd++] = key;
				
				//save_in_disk(buffer, key, keylen, &memory_usage, memory_availability, graphs_fd);
				//fwrite(key, 1, keylen + 1, graphs_fd);
				e++;
				//free(key);
				key = brz_read_key(tmp_fds[i]);
			}
			if (key)
			{
				//save_in_disk(buffer, buffer_merge[i], strlen(buffer_merge[i]), &memory_usage, memory_availability, graphs_fd);
				assert(nkeys_vd < brz->size[cur_bucket]);
				keys_vd[nkeys_vd++] = buffer_merge[i];
				//fwrite(buffer_merge[i], 1, strlen(buffer_merge[i]) + 1, graphs_fd);
				e++;
				buffer_h3[i] = h3;
				//free(buffer_merge[i]);
				buffer_merge[i] = (cmph_uint8 *)calloc(keylen + 1, sizeof(cmph_uint8));
				memcpy(buffer_merge[i], key, keylen + 1);
				free(key);
			}
		}
/*		fprintf(stderr, "BOSTA %u  %u  e: %u\n", i, buffer_h3[i], e);*/
		if(!key)
		{
			assert(nkeys_vd < brz->size[cur_bucket]);
			keys_vd[nkeys_vd++] = buffer_merge[i];
			//save_in_disk(buffer, buffer_merge[i], strlen(buffer_merge[i]), &memory_usage, memory_availability, graphs_fd);
			//fwrite(buffer_merge[i], 1, strlen(buffer_merge[i]) + 1, graphs_fd);
			e++;
			buffer_h3[i] = UINT_MAX;
			//free(buffer_merge[i]);
			buffer_merge[i] = NULL;
		}
		
		if(nkeys_vd == brz->size[cur_bucket]) // Generating mphf.
		{
			cmph_io_adapter_t *source = NULL;
			cmph_config_t *config = NULL;
			cmph_t *mphf_tmp = NULL;
			bmz_data_t * bmzf = NULL;
			// Source of keys
			//fprintf(stderr, "Generating mphf %u in %u \n",cur_bucket + 1, brz->k);
			source = cmph_io_vector_adapter(keys_vd, (cmph_uint32)nkeys_vd);
			config = cmph_config_new(source);
			cmph_config_set_algo(config, CMPH_BMZ);
			cmph_config_set_graphsize(config, brz->c);
			mphf_tmp = cmph_new(config);
			bmzf = (bmz_data_t *)mphf_tmp->data;
			//assert(brz_verify_mphf(mphf_tmp, source));
			brz_copy_partial_mphf(brz, bmzf, cur_bucket, source);
			cmph_config_destroy(config);
			brz_destroy_keys_vd(keys_vd, nkeys_vd);
			cmph_destroy(mphf_tmp);
			free(source);
			nkeys_vd = 0;
		}
	}
	for(i = 0; i < nflushes; i++) fclose(tmp_fds[i]);
	//flush_buffer(buffer, &memory_usage, graphs_fd);
	free(tmp_fds);
	free(keys_vd);
	free(buffer_merge);
	free(buffer_h3);
	return 1;
#pragma pack()
}

static void flush_buffer(cmph_uint8 *buffer, cmph_uint32 *memory_usage, FILE * graphs_fd)
{
	fwrite(buffer, 1, *memory_usage, graphs_fd);
	*memory_usage = 0;
}
			 
static void save_in_disk(cmph_uint8 *buffer, cmph_uint8 * key, cmph_uint32 keylen, cmph_uint32 * memory_usage, 
			 cmph_uint32 memory_availability, FILE * graphs_fd)
{
	if(*memory_usage + keylen + 1 > memory_availability)
	{
		flush_buffer(buffer, memory_usage, graphs_fd);
	}
	memcpy(buffer + *memory_usage, key, keylen + 1);
	*memory_usage = *memory_usage + keylen + 1;
}

static cmph_uint32 brz_min_index(cmph_uint32 * vector, cmph_uint32 n)
{
	cmph_uint32 i, min_index = 0;
	for(i = 1; i < n; i++)
	{
		if(vector[i] < vector[min_index]) min_index = i;
	}
	return min_index;
}

static char * brz_read_key(FILE * fd)
{
	char * buf = (char *)malloc(BUFSIZ);
	cmph_uint32 buf_pos = 0;
	char c;
	while(1)
	{
		fread(&c, sizeof(char), 1, fd);
		if(feof(fd))
		{
			free(buf);
			return NULL;
		}
		buf[buf_pos++] = c;
		if(c == '\0') break;
		if(buf_pos % BUFSIZ == 0) buf = (char *)realloc(buf, buf_pos + BUFSIZ);
	}
	return buf;
}

static char ** brz_read_keys_vd(FILE * graphs_fd, cmph_uint8 nkeys)
{
	char ** keys_vd = (char **)malloc(sizeof(char *)*nkeys);
	cmph_uint8 i;
	
	for(i = 0; i < nkeys; i++)
	{
		char * buf = brz_read_key(graphs_fd);
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
