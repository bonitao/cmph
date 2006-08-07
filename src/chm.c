#include "graph.h"
#include "chm.h"
#include "cmph_structs.h"
#include "chm_structs.h"
#include "hash.h"
#include "bitbool.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

//#define DEBUG
#include "debug.h"

static int chm_gen_edges(cmph_config_t *mph);
static void chm_traverse(chm_config_data_t *chm, cmph_uint8 *visited, cmph_uint32 v);

chm_config_data_t *chm_config_new()
{
	chm_config_data_t *chm;
	chm = (chm_config_data_t *)malloc(sizeof(chm_config_data_t));
	assert(chm);
	memset(chm,0,sizeof(chm_config_data_t));
	chm->hashfuncs[0] = CMPH_HASH_JENKINS;
	chm->hashfuncs[1] = CMPH_HASH_JENKINS;
	chm->g = NULL;
	chm->graph = NULL;
	chm->hashes = NULL;
	return chm;
}
void chm_config_destroy(cmph_config_t *mph)
{
	chm_config_data_t *data = (chm_config_data_t *)mph->data;
	DEBUGP("Destroying algorithm dependent data\n");
	free(data);
}

void chm_config_set_hashfuncs(cmph_config_t *mph, CMPH_HASH *hashfuncs)
{
	chm_config_data_t *chm = (chm_config_data_t *)mph->data;
	CMPH_HASH *hashptr = hashfuncs;
	cmph_uint32 i = 0;
	while(*hashptr != CMPH_HASH_COUNT)
	{
		if (i >= 2) break; //chm only uses two hash functions
		chm->hashfuncs[i] = *hashptr;	
		++i, ++hashptr;
	}
}

cmph_t *chm_new(cmph_config_t *mph, float c)
{
	cmph_t *mphf = NULL;
	chm_data_t *chmf = NULL;

	cmph_uint32 i;
	cmph_uint32 iterations = 20;
	cmph_uint8 *visited = NULL;
	chm_config_data_t *chm = (chm_config_data_t *)mph->data;
	chm->m = mph->key_source->nkeys;
	if (c == 0) c = 2.09;
	chm->n = ceil(c * mph->key_source->nkeys);	
	DEBUGP("m (edges): %u n (vertices): %u c: %f\n", chm->m, chm->n, c);
	chm->graph = graph_new(chm->n, chm->m);
	DEBUGP("Created graph\n");

	chm->hashes = (hash_state_t **)malloc(sizeof(hash_state_t *)*3);
	for(i = 0; i < 3; ++i) chm->hashes[i] = NULL;
	//Mapping step
	if (mph->verbosity)
	{
		fprintf(stderr, "Entering mapping step for mph creation of %u keys with graph sized %u\n", chm->m, chm->n);
	}
	while(1)
	{
		int ok;
		chm->hashes[0] = hash_state_new(chm->hashfuncs[0], chm->n);
		chm->hashes[1] = hash_state_new(chm->hashfuncs[1], chm->n);
		ok = chm_gen_edges(mph);
		if (!ok)
		{
			--iterations;
			hash_state_destroy(chm->hashes[0]);
			chm->hashes[0] = NULL;
			hash_state_destroy(chm->hashes[1]);
			chm->hashes[1] = NULL;
			DEBUGP("%u iterations remaining\n", iterations);
			if (mph->verbosity)
			{
				fprintf(stderr, "Acyclic graph creation failure - %u iterations remaining\n", iterations);
			}
			if (iterations == 0) break;
		} 
		else break;	
	}
	if (iterations == 0)
	{
		graph_destroy(chm->graph);	
		return NULL;
	}

	//Assignment step
	if (mph->verbosity)
	{
		fprintf(stderr, "Starting assignment step\n");
	}
	DEBUGP("Assignment step\n");
 	visited = (cmph_uint8 *)malloc(chm->n/8 + 1);
	memset(visited, 0, chm->n/8 + 1);
	free(chm->g);
	chm->g = (cmph_uint32 *)malloc(chm->n * sizeof(cmph_uint32));
	assert(chm->g);
	for (i = 0; i < chm->n; ++i)
	{
	        if (!GETBIT(visited,i))
		{
			chm->g[i] = 0;
			chm_traverse(chm, visited, i);
		}
	}
	graph_destroy(chm->graph);	
	free(visited);
	chm->graph = NULL;

	mphf = (cmph_t *)malloc(sizeof(cmph_t));
	mphf->algo = mph->algo;
	chmf = (chm_data_t *)malloc(sizeof(chm_data_t));
	chmf->g = chm->g;
	chm->g = NULL; //transfer memory ownership
	chmf->hashes = chm->hashes;
	chm->hashes = NULL; //transfer memory ownership
	chmf->n = chm->n;
	chmf->m = chm->m;
	mphf->data = chmf;
	mphf->size = chm->m;
	DEBUGP("Successfully generated minimal perfect hash\n");
	if (mph->verbosity)
	{
		fprintf(stderr, "Successfully generated minimal perfect hash function\n");
	}
	return mphf;
}

static void chm_traverse(chm_config_data_t *chm, cmph_uint8 *visited, cmph_uint32 v)
{

	graph_iterator_t it = graph_neighbors_it(chm->graph, v);
	cmph_uint32 neighbor = 0;
	SETBIT(visited,v);
	
	DEBUGP("Visiting vertex %u\n", v);
	while((neighbor = graph_next_neighbor(chm->graph, &it)) != GRAPH_NO_NEIGHBOR)
	{
		DEBUGP("Visiting neighbor %u\n", neighbor);
		if(GETBIT(visited,neighbor)) continue;
		DEBUGP("Visiting neighbor %u\n", neighbor);
		DEBUGP("Visiting edge %u->%u with id %u\n", v, neighbor, graph_edge_id(chm->graph, v, neighbor));
		chm->g[neighbor] = graph_edge_id(chm->graph, v, neighbor) - chm->g[v];
		DEBUGP("g is %u (%u - %u mod %u)\n", chm->g[neighbor], graph_edge_id(chm->graph, v, neighbor), chm->g[v], chm->m);
		chm_traverse(chm, visited, neighbor);
	}
}
		
static int chm_gen_edges(cmph_config_t *mph)
{
	cmph_uint32 e;
	chm_config_data_t *chm = (chm_config_data_t *)mph->data;
	int cycles = 0;

	DEBUGP("Generating edges for %u vertices with hash functions %s and %s\n", chm->n, cmph_hash_names[chm->hashfuncs[0]], cmph_hash_names[chm->hashfuncs[1]]);
	graph_clear_edges(chm->graph);	
	mph->key_source->rewind(mph->key_source->data);
	for (e = 0; e < mph->key_source->nkeys; ++e)
	{
		cmph_uint32 h1, h2;
		cmph_uint32 keylen;
		char *key;
		mph->key_source->read(mph->key_source->data, &key, &keylen);
		h1 = hash(chm->hashes[0], key, keylen) % chm->n;
		h2 = hash(chm->hashes[1], key, keylen) % chm->n;
		if (h1 == h2) if (++h2 >= chm->n) h2 = 0;
		if (h1 == h2) 
		{
			if (mph->verbosity) fprintf(stderr, "Self loop for key %u\n", e);
			mph->key_source->dispose(mph->key_source->data, key, keylen);
			return 0;
		}
		DEBUGP("Adding edge: %u -> %u for key %s\n", h1, h2, key);
		mph->key_source->dispose(mph->key_source->data, key, keylen);
		graph_add_edge(chm->graph, h1, h2);
	}
	cycles = graph_is_cyclic(chm->graph);
	if (mph->verbosity && cycles) fprintf(stderr, "Cyclic graph generated\n");
	DEBUGP("Looking for cycles: %u\n", cycles);

	return ! cycles;
}

int chm_dump(cmph_t *mphf, FILE *fd)
{
	char *buf = NULL;
	cmph_uint32 buflen;
	cmph_uint32 two = 2; //number of hash functions
	chm_data_t *data = (chm_data_t *)mphf->data;
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
	
	fwrite(data->g, sizeof(cmph_uint32)*data->n, 1, fd);
	#ifdef DEBUG
	fprintf(stderr, "G: ");
	for (i = 0; i < data->n; ++i) fprintf(stderr, "%u ", data->g[i]);
	fprintf(stderr, "\n");
	#endif
	return 1;
}

void chm_load(FILE *f, cmph_t *mphf)
{
	cmph_uint32 nhashes;
	char *buf = NULL;
	cmph_uint32 buflen;
	cmph_uint32 i;
	chm_data_t *chm = (chm_data_t *)malloc(sizeof(chm_data_t));

	DEBUGP("Loading chm mphf\n");
	mphf->data = chm;
	fread(&nhashes, sizeof(cmph_uint32), 1, f);
	chm->hashes = (hash_state_t **)malloc(sizeof(hash_state_t *)*(nhashes + 1));
	chm->hashes[nhashes] = NULL;
	DEBUGP("Reading %u hashes\n", nhashes);
	for (i = 0; i < nhashes; ++i)
	{
		hash_state_t *state = NULL;
		fread(&buflen, sizeof(cmph_uint32), 1, f);
		DEBUGP("Hash state has %u bytes\n", buflen);
		buf = (char *)malloc(buflen);
		fread(buf, buflen, 1, f);
		state = hash_state_load(buf, buflen);
		chm->hashes[i] = state;
		free(buf);
	}

	DEBUGP("Reading m and n\n");
	fread(&(chm->n), sizeof(cmph_uint32), 1, f);	
	fread(&(chm->m), sizeof(cmph_uint32), 1, f);	

	chm->g = (cmph_uint32 *)malloc(sizeof(cmph_uint32)*chm->n);
	fread(chm->g, chm->n*sizeof(cmph_uint32), 1, f);
	#ifdef DEBUG
	fprintf(stderr, "G: ");
	for (i = 0; i < chm->n; ++i) fprintf(stderr, "%u ", chm->g[i]);
	fprintf(stderr, "\n");
	#endif
	return;
}
		

cmph_uint32 chm_search(cmph_t *mphf, const char *key, cmph_uint32 keylen)
{
	chm_data_t *chm = mphf->data;
	cmph_uint32 h1 = hash(chm->hashes[0], key, keylen) % chm->n;
	cmph_uint32 h2 = hash(chm->hashes[1], key, keylen) % chm->n;
	DEBUGP("key: %s h1: %u h2: %u\n", key, h1, h2);
	if (h1 == h2 && ++h2 >= chm->n) h2 = 0;
	DEBUGP("key: %s g[h1]: %u g[h2]: %u edges: %u\n", key, chm->g[h1], chm->g[h2], chm->m);
	return (chm->g[h1] + chm->g[h2]) % chm->m;
}
void chm_destroy(cmph_t *mphf)
{
	chm_data_t *data = (chm_data_t *)mphf->data;
	free(data->g);	
	hash_state_destroy(data->hashes[0]);
	hash_state_destroy(data->hashes[1]);
	free(data->hashes);
	free(data);
	free(mphf);
}
