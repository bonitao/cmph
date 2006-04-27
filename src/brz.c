#include "graph.h"
#include "bmz8.h"
#include "bmz8_structs.h"
#include "brz.h"
#include "cmph_structs.h"
#include "brz_structs.h"
#include "buffer_manage.h"
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

static int brz_gen_mphf(cmph_config_t *mph);
static cmph_uint32 brz_min_index(cmph_uint32 * vector, cmph_uint32 n);
static void brz_destroy_keys_vd(char ** keys_vd, cmph_uint8 nkeys);
static char * brz_copy_partial_mphf(brz_config_data_t *brz, bmz8_data_t * bmzf, cmph_uint32 index,  cmph_uint32 *buflen);
brz_config_data_t *brz_config_new()
{
	brz_config_data_t *brz = NULL; 	
	brz = (brz_config_data_t *)malloc(sizeof(brz_config_data_t));
	brz->b = 128;
	brz->hashfuncs[0] = CMPH_HASH_JENKINS;
	brz->hashfuncs[1] = CMPH_HASH_JENKINS;
	brz->hashfuncs[2] = CMPH_HASH_JENKINS;
	brz->size   = NULL;
	brz->offset = NULL;
	brz->g      = NULL;
	brz->h1 = NULL;
	brz->h2 = NULL;
	brz->h0 = NULL;
	brz->memory_availability = 1024*1024;
	brz->tmp_dir = (cmph_uint8 *)calloc(10, sizeof(cmph_uint8));
	brz->mphf_fd = NULL;
	strcpy((char *)(brz->tmp_dir), "/var/tmp/"); 
	assert(brz);
	return brz;
}

void brz_config_destroy(cmph_config_t *mph)
{
	brz_config_data_t *data = (brz_config_data_t *)mph->data;
	free(data->tmp_dir);
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

void brz_config_set_memory_availability(cmph_config_t *mph, cmph_uint32 memory_availability)
{
	brz_config_data_t *brz = (brz_config_data_t *)mph->data;
	if(memory_availability > 0) brz->memory_availability = memory_availability*1024*1024;
}

void brz_config_set_tmp_dir(cmph_config_t *mph, cmph_uint8 *tmp_dir)
{
	brz_config_data_t *brz = (brz_config_data_t *)mph->data;
	if(tmp_dir)
	{
		cmph_uint32 len = strlen((char *)tmp_dir);
		free(brz->tmp_dir);
		if(tmp_dir[len-1] != '/')
		{
			brz->tmp_dir = calloc(len+2, sizeof(cmph_uint8));
			sprintf((char *)(brz->tmp_dir), "%s/", (char *)tmp_dir); 
		}
		else
		{
			brz->tmp_dir = calloc(len+1, sizeof(cmph_uint8));
			sprintf((char *)(brz->tmp_dir), "%s", (char *)tmp_dir); 
		}
		
	}
}

void brz_config_set_mphf_fd(cmph_config_t *mph, FILE *mphf_fd)
{
	brz_config_data_t *brz = (brz_config_data_t *)mph->data;
	brz->mphf_fd = mphf_fd;
	assert(brz->mphf_fd);
}

void brz_config_set_b(cmph_config_t *mph, cmph_uint8 b)
{
	brz_config_data_t *brz = (brz_config_data_t *)mph->data;
	brz->b = b;
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
	brz->k = ceil(brz->m/(brz->b));
	DEBUGP("k: %u\n", brz->k);
	brz->size   = (cmph_uint8 *) calloc(brz->k, sizeof(cmph_uint8));
	
	// Clustering the keys by graph id.
	if (mph->verbosity)
	{
		fprintf(stderr, "Partioning the set of keys.\n");	
	}
		
	while(1)
	{
		int ok;
		DEBUGP("hash function 3\n");
		brz->h0 = hash_state_new(brz->hashfuncs[2], brz->k);
		DEBUGP("Generating graphs\n");
		ok = brz_gen_mphf(mph);
		if (!ok)
		{
			--iterations;
			hash_state_destroy(brz->h0);
			brz->h0 = NULL;
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
	brzf->h0 = brz->h0;
	brz->h0 = NULL; //transfer memory ownership
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

static int brz_gen_mphf(cmph_config_t *mph)
{
	cmph_uint32 i, e;
	brz_config_data_t *brz = (brz_config_data_t *)mph->data;
	cmph_uint32 memory_usage = 0;
	cmph_uint32 nkeys_in_buffer = 0;
	cmph_uint8 *buffer = (cmph_uint8 *)malloc(brz->memory_availability);
	cmph_uint32 *buckets_size = (cmph_uint32 *)calloc(brz->k, sizeof(cmph_uint32));
	cmph_uint32 *keys_index = NULL;
	cmph_uint8 **buffer_merge = NULL;
	cmph_uint32 *buffer_h0 = NULL;
	cmph_uint32 nflushes = 0;
	cmph_uint32 h0;
	FILE *  tmp_fd = NULL;
	buffer_manage_t * buff_manage = NULL;
	char *filename = NULL;
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
		if (memory_usage + keylen + 1 > brz->memory_availability) // flush buffers 
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
				keylen1 = strlen((char *)(buffer + memory_usage));
				h0 = hash(brz->h0, (char *)(buffer + memory_usage), keylen1) % brz->k;
				keys_index[buckets_size[h0]] = memory_usage;
				buckets_size[h0]++;
				memory_usage = memory_usage + keylen1 + 1;
			}
			filename = (char *)calloc(strlen((char *)(brz->tmp_dir)) + 11, sizeof(char));
			sprintf(filename, "%s%u.cmph",brz->tmp_dir, nflushes);
			tmp_fd = fopen(filename, "wb");
			free(filename);
			filename = NULL;
			for(i = 0; i < nkeys_in_buffer; i++)
			{
				keylen1 = strlen((char *)(buffer + keys_index[i])) + 1;
				fwrite(buffer + keys_index[i], 1, keylen1, tmp_fd);
			}
			nkeys_in_buffer = 0;
			memory_usage = 0;
			bzero(buckets_size, brz->k*sizeof(cmph_uint32));
			nflushes++;
			free(keys_index);
			fclose(tmp_fd);
		}
		memcpy(buffer + memory_usage, key, keylen + 1);
		memory_usage = memory_usage + keylen + 1;
		h0 = hash(brz->h0, key, keylen) % brz->k;
		if ((brz->size[h0] == MAX_BUCKET_SIZE) || ((brz->c >= 1.0) && (cmph_uint8)(brz->c * brz->size[h0]) < brz->size[h0])) 
		{
			free(buffer);
			free(buckets_size);
			return 0;
		}
		brz->size[h0] = brz->size[h0] + 1;
		buckets_size[h0] ++;
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
			keylen1 = strlen((char *)(buffer + memory_usage));
			h0 = hash(brz->h0, (char *)(buffer + memory_usage), keylen1) % brz->k;
			keys_index[buckets_size[h0]] = memory_usage;
			buckets_size[h0]++;
			memory_usage = memory_usage + keylen1 + 1;
		}
		filename = (char *)calloc(strlen((char *)(brz->tmp_dir)) + 11, sizeof(char));
		sprintf(filename, "%s%u.cmph",brz->tmp_dir, nflushes);
		tmp_fd = fopen(filename, "wb");
		free(filename);
		filename = NULL;
		for(i = 0; i < nkeys_in_buffer; i++)
		{
			keylen1 = strlen((char *)(buffer + keys_index[i])) + 1;
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
	/* Starting to dump to disk the resultant MPHF: __cmph_dump function */
	fwrite(cmph_names[CMPH_BRZ], (cmph_uint32)(strlen(cmph_names[CMPH_BRZ]) + 1), 1, brz->mphf_fd);
	fwrite(&(brz->m), sizeof(brz->m), 1, brz->mphf_fd);
	fwrite(&(brz->c), sizeof(cmph_float32), 1, brz->mphf_fd);
	fwrite(&(brz->k), sizeof(cmph_uint32), 1, brz->mphf_fd); // number of MPHFs
	fwrite(brz->size, sizeof(cmph_uint8)*(brz->k), 1, brz->mphf_fd);
	
	//tmp_fds = (FILE **)calloc(nflushes, sizeof(FILE *));
	buff_manage = buffer_manage_new(brz->memory_availability, nflushes);
	buffer_merge = (cmph_uint8 **)calloc(nflushes, sizeof(cmph_uint8 *));
	buffer_h0    = (cmph_uint32 *)calloc(nflushes, sizeof(cmph_uint32));
	
	memory_usage = 0;
	for(i = 0; i < nflushes; i++)
	{
		filename = (char *)calloc(strlen((char *)(brz->tmp_dir)) + 11, sizeof(char));
		sprintf(filename, "%s%u.cmph",brz->tmp_dir, i);
		buffer_manage_open(buff_manage, i, filename);
		free(filename);
		filename = NULL;
		key = (char *)buffer_manage_read_key(buff_manage, i);
		keylen = strlen(key);
		h0 = hash(brz->h0, key, keylen) % brz->k;
		buffer_h0[i] = h0;
		buffer_merge[i] = (cmph_uint8 *)calloc(keylen + 1, sizeof(cmph_uint8));
		memcpy(buffer_merge[i], key, keylen + 1);
		free(key);
	}
	e = 0;
	keys_vd = (char **)calloc(MAX_BUCKET_SIZE, sizeof(char *));
	nkeys_vd = 0;
	while(e < brz->m)
	{
		i = brz_min_index(buffer_h0, nflushes);
		cur_bucket = buffer_h0[i];
		key = (char *)buffer_manage_read_key(buff_manage, i);
		if(key)
		{
			while(key)
			{
				keylen = strlen(key);
				h0 = hash(brz->h0, key, keylen) % brz->k;				
				if (h0 != buffer_h0[i]) break;				
				keys_vd[nkeys_vd++] = key;
				key = NULL; //transfer memory ownership
				e++;
				key = (char *)buffer_manage_read_key(buff_manage, i);
			}
			if (key)
			{
				assert(nkeys_vd < brz->size[cur_bucket]);
				keys_vd[nkeys_vd++] = (char *)buffer_merge[i];
				buffer_merge[i] = NULL; //transfer memory ownership
				e++;
				buffer_h0[i] = h0;
				buffer_merge[i] = (cmph_uint8 *)calloc(keylen + 1, sizeof(cmph_uint8));
				memcpy(buffer_merge[i], key, keylen + 1);
				free(key);
			}
		}
		if(!key)
		{
			assert(nkeys_vd < brz->size[cur_bucket]);
			keys_vd[nkeys_vd++] = (char *)buffer_merge[i];
			buffer_merge[i] = NULL; //transfer memory ownership
			e++;
			buffer_h0[i] = UINT_MAX;
		}
		
		if(nkeys_vd == brz->size[cur_bucket]) // Generating mphf for each bucket.
		{
			cmph_io_adapter_t *source = NULL;
			cmph_config_t *config = NULL;
			cmph_t *mphf_tmp = NULL;
			bmz8_data_t * bmzf = NULL;
			char *bufmphf = NULL;
			cmph_uint32 buflenmphf = 0;
			// Source of keys
			source = cmph_io_vector_adapter(keys_vd, (cmph_uint32)nkeys_vd);
			config = cmph_config_new(source);
			cmph_config_set_algo(config, CMPH_BMZ8);
			cmph_config_set_graphsize(config, brz->c);
			mphf_tmp = cmph_new(config);
			bmzf = (bmz8_data_t *)mphf_tmp->data;
			bufmphf = brz_copy_partial_mphf(brz, bmzf, cur_bucket,  &buflenmphf);
			bmzf = NULL;
		        fwrite(bufmphf, buflenmphf, 1, brz->mphf_fd);
			free(bufmphf);
			bufmphf = NULL;
			cmph_config_destroy(config);
 			brz_destroy_keys_vd(keys_vd, nkeys_vd);
			cmph_destroy(mphf_tmp);
			cmph_io_vector_adapter_destroy(source);
			
			nkeys_vd = 0;
		}
	}

	buffer_manage_destroy(buff_manage);
	free(keys_vd);
	free(buffer_merge);
	free(buffer_h0);
	return 1;
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

static void brz_destroy_keys_vd(char ** keys_vd, cmph_uint8 nkeys)
{
	cmph_uint8 i;
	for(i = 0; i < nkeys; i++) { free(keys_vd[i]); keys_vd[i] = NULL;}
}

static char * brz_copy_partial_mphf(brz_config_data_t *brz, bmz8_data_t * bmzf, cmph_uint32 index,  cmph_uint32 *buflen)
{
	cmph_uint32 buflenh1 = 0;
	cmph_uint32 buflenh2 = 0; 
	char * bufh1 = NULL;
	char * bufh2 = NULL;
	char * buf   = NULL;
	cmph_uint32 n = ceil(brz->c * brz->size[index]);
	hash_state_dump(bmzf->hashes[0], &bufh1, &buflenh1);
	hash_state_dump(bmzf->hashes[1], &bufh2, &buflenh2);
	*buflen = buflenh1 + buflenh2 + n + 2*sizeof(cmph_uint32);
	buf = (char *)malloc(*buflen);
	//fprintf(stderr,"entrei passei\n");
	memcpy(buf, &buflenh1, sizeof(cmph_uint32));
	memcpy(buf+sizeof(cmph_uint32), bufh1, buflenh1);
	memcpy(buf+sizeof(cmph_uint32)+buflenh1, &buflenh2, sizeof(cmph_uint32));
	memcpy(buf+2*sizeof(cmph_uint32)+buflenh1, bufh2, buflenh2);
	memcpy(buf+2*sizeof(cmph_uint32)+buflenh1+buflenh2,bmzf->g, n);
	free(bufh1);
	free(bufh2);
	return buf;
}
int brz_dump(cmph_t *mphf, FILE *fd)
{
	brz_data_t *data = (brz_data_t *)mphf->data;
	char *buf = NULL;
	cmph_uint32 buflen;
	DEBUGP("Dumping brzf\n");
	// The initial part of the MPHF have already been dumped to disk during construction
	// Dumping h0
        hash_state_dump(data->h0, &buf, &buflen);
        DEBUGP("Dumping hash state with %u bytes to disk\n", buflen);
        fwrite(&buflen, sizeof(cmph_uint32), 1, fd);
        fwrite(buf, buflen, 1, fd);
        free(buf);
	// Dumping m and the vector offset.
	fwrite(&(data->m), sizeof(cmph_uint32), 1, fd);	
	fwrite(data->offset, sizeof(cmph_uint32)*(data->k), 1, fd);
	return 1;
}




void brz_load(FILE *f, cmph_t *mphf)
{
	char *buf = NULL;
	cmph_uint32 buflen;
	cmph_uint32 i, n;
	brz_data_t *brz = (brz_data_t *)malloc(sizeof(brz_data_t));

	DEBUGP("Loading brz mphf\n");
	mphf->data = brz;
	fread(&(brz->c), sizeof(cmph_float32), 1, f);
	fread(&(brz->k), sizeof(cmph_uint32), 1, f);
	brz->size   = (cmph_uint8 *) malloc(sizeof(cmph_uint8)*brz->k);
	fread(brz->size, sizeof(cmph_uint8)*(brz->k), 1, f);	
	brz->h1 = (hash_state_t **)malloc(sizeof(hash_state_t *)*brz->k);
	brz->h2 = (hash_state_t **)malloc(sizeof(hash_state_t *)*brz->k);
	brz->g  = (cmph_uint8 **)  calloc(brz->k, sizeof(cmph_uint8 *));
	DEBUGP("Reading %u h1 and %u h2\n", brz->k, brz->k);
	//loading h_i1, h_i2 and g_i.
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
		n = ceil(brz->c * brz->size[i]);
		DEBUGP("g_i has %u bytes\n", n);
		brz->g[i] = (cmph_uint8 *)calloc(n, sizeof(cmph_uint8));
		fread(brz->g[i], sizeof(cmph_uint8)*n, 1, f);
	}
	//loading h0
	fread(&buflen, sizeof(cmph_uint32), 1, f);
	DEBUGP("Hash state has %u bytes\n", buflen);
	buf = (char *)malloc(buflen);
	fread(buf, buflen, 1, f);
	brz->h0 = hash_state_load(buf, buflen);
	free(buf);

	//loading c, m, and the vector offset.	
	fread(&(brz->m), sizeof(cmph_uint32), 1, f);
	brz->offset = (cmph_uint32 *)malloc(sizeof(cmph_uint32)*brz->k);
	fread(brz->offset, sizeof(cmph_uint32)*(brz->k), 1, f);	
	return;
}

cmph_uint32 brz_search(cmph_t *mphf, const char *key, cmph_uint32 keylen)
{
	brz_data_t *brz = mphf->data;	
	cmph_uint32 h0 = hash(brz->h0, key, keylen) % brz->k;
	cmph_uint32 m = brz->size[h0];
	cmph_uint32 n = ceil(brz->c * m);
	cmph_uint32 h1 = hash(brz->h1[h0], key, keylen) % n;
	cmph_uint32 h2 = hash(brz->h2[h0], key, keylen) % n;
	cmph_uint8 mphf_bucket;
	if (h1 == h2 && ++h2 >= n) h2 = 0;
	mphf_bucket = brz->g[h0][h1] + brz->g[h0][h2]; 
	DEBUGP("key: %s h1: %u h2: %u h0: %u\n", key, h1, h2, h0);
	DEBUGP("key: %s g[h1]: %u g[h2]: %u offset[h0]: %u edges: %u\n", key, brz->g[h0][h1], brz->g[h0][h2], brz->offset[h0], brz->m);
	DEBUGP("Address: %u\n", mphf_bucket + brz->offset[h0]);
	return (mphf_bucket + brz->offset[h0]);
}
void brz_destroy(cmph_t *mphf)
{
	cmph_uint32 i;
	brz_data_t *data = (brz_data_t *)mphf->data;
	if(data->g)
	{
		for(i = 0; i < data->k; i++)
		{
			free(data->g[i]);
			hash_state_destroy(data->h1[i]);
			hash_state_destroy(data->h2[i]);
		}
		free(data->g);
		free(data->h1);
		free(data->h2);
	}
	hash_state_destroy(data->h0);
	free(data->size);
	free(data->offset);
	free(data);
	free(mphf);
}
