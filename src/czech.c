#include "czech.h"
#include "cmph_structs.h"
#include "czech_structs.h"
#include "hash.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <netinet/in.h>

//#define DEBUG
#include "debug.h"

static int czech_gen_edges(mph_t *mph);
static void czech_traverse(czech_mph_data_t *czech, char *visited, uint32 v);

mph_t *czech_mph_new(key_source_t *key_source)
{
	mph_t *mph = NULL;
	czech_mph_data_t *czech = NULL; 	
	mph = __mph_new(MPH_CZECH, key_source);
	if (mph == NULL) return NULL;
	czech = (czech_mph_data_t *)malloc(sizeof(czech_mph_data_t));
	if (czech == NULL)
	{
		__mph_destroy(mph);
		return NULL;
	}
	czech->hashfuncs[0] = HASH_JENKINS;
	czech->hashfuncs[1] = HASH_JENKINS;
	czech->g = NULL;
	czech->graph = NULL;
	czech->hashes = NULL;
	mph->data = czech;
	assert(mph->data);
	return mph;
}
void czech_mph_destroy(mph_t *mph)
{
	czech_mph_data_t *data = (czech_mph_data_t *)mph->data;
	DEBUGP("Destroying algorithm dependent data\n");
	free(data);
	__mph_destroy(mph);
}

void czech_mph_set_hashfuncs(mph_t *mph, CMPH_HASH *hashfuncs)
{
	czech_mph_data_t *czech = (czech_mph_data_t *)mph->data;
	CMPH_HASH *hashptr = hashfuncs;
	uint32 i = 0;
	while(*hashptr != HASH_COUNT)
	{
		if (i >= 2) break; //czech only uses two hash functions
		czech->hashfuncs[i] = *hashptr;	
		++i, ++hashptr;
	}
}

mphf_t *czech_mph_create(mph_t *mph, float c)
{
	mphf_t *mphf = NULL;
	czech_mphf_data_t *czechf = NULL;

	uint32 i;
	uint32 iterations = 10;
	char *visited = NULL;
	czech_mph_data_t *czech = (czech_mph_data_t *)mph->data;
	czech->m = mph->key_source->nkeys;	
	czech->n = ceil(c * mph->key_source->nkeys);	
	DEBUGP("m (edges): %u n (vertices): %u c: %f\n", czech->m, czech->n, c);
	czech->graph = graph_new(czech->n, czech->m);
	DEBUGP("Created graph\n");

	czech->hashes = (hash_state_t **)malloc(sizeof(hash_state_t *)*3);
	for(i = 0; i < 3; ++i) czech->hashes[i] = NULL;
	//Mapping step
	if (mph->verbosity)
	{
		fprintf(stderr, "Entering mapping step for mph creation of %u keys with graph sized %u\n", czech->m, czech->n);
	}
	while(1)
	{
		int ok;
		czech->hashes[0] = hash_state_new(czech->hashfuncs[0], czech->n);
		czech->hashes[1] = hash_state_new(czech->hashfuncs[1], czech->n);
		ok = czech_gen_edges(mph);
		if (!ok)
		{
			--iterations;
			hash_state_destroy(czech->hashes[0]);
			czech->hashes[0] = NULL;
			hash_state_destroy(czech->hashes[1]);
			czech->hashes[1] = NULL;
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
		graph_destroy(czech->graph);	
		return NULL;
	}

	//Assignment step
	if (mph->verbosity)
	{
		fprintf(stderr, "Starting assignment step\n");
	}
	DEBUGP("Assignment step\n");
	visited = (char *)malloc(czech->n);
	memset(visited, 0, czech->n);
	free(czech->g);
	czech->g = malloc(czech->n * sizeof(uint32));
	assert(czech->g);
	if (!czech->g)
	{
		fprintf(stderr, "out of memory");
		free(visited);
		graph_destroy(czech->graph);
		return NULL;
	}
	for (i = 0; i < czech->n; ++i)
	{
		if (!visited[i])
		{
			czech->g[i] = 0;
			czech_traverse(czech, visited, i);
		}
	}
	graph_destroy(czech->graph);	
	free(visited);
	czech->graph = NULL;

	mphf = (mphf_t *)malloc(sizeof(mphf_t));
	mphf->algo = mph->algo;
	czechf = (czech_mphf_data_t *)malloc(sizeof(czech_mph_data_t));
	czechf->g = czech->g;
	czech->g = NULL; //transfer memory ownership
	czechf->hashes = czech->hashes;
	czech->hashes = NULL; //transfer memory ownership
	czechf->n = czech->n;
	czechf->m = czech->m;
	mphf->data = czechf;
	mphf->size = czech->m;
	DEBUGP("Successfully generated minimal perfect hash\n");
	if (mph->verbosity)
	{
		fprintf(stderr, "Successfully generated minimal perfect hash function\n");
	}
	return mphf;
}

static void czech_traverse(czech_mph_data_t *czech, char *visited, uint32 v)
{

	graph_iterator_t it = graph_neighbors_it(czech->graph, v);
	uint32 neighbor = 0;
	visited[v] = 1;
	
	DEBUGP("Visiting vertex %u\n", v);
	while((neighbor = graph_next_neighbor(czech->graph, &it)) != GRAPH_NO_NEIGHBOR)
	{
		DEBUGP("Visiting neighbor %u\n", neighbor);
		if(visited[neighbor]) continue;
		DEBUGP("Visiting neighbor %u\n", neighbor);
		DEBUGP("Visiting edge %u->%u with id %u\n", v, neighbor, graph_edge_id(czech->graph, v, neighbor));
		czech->g[neighbor] = graph_edge_id(czech->graph, v, neighbor) - czech->g[v];
		DEBUGP("g is %u (%u - %u mod %u)\n", czech->g[neighbor], graph_edge_id(czech->graph, v, neighbor), czech->g[v], czech->m);
		czech_traverse(czech, visited, neighbor);
	}
}
		
static int czech_gen_edges(mph_t *mph)
{
	uint32 e;
	czech_mph_data_t *czech = (czech_mph_data_t *)mph->data;
	int cycles = 0;

	DEBUGP("Generating edges for %u vertices\n", czech->n);
	graph_clear_edges(czech->graph);	
	mph->key_source->rewind(mph->key_source->data);
	for (e = 0; e < mph->key_source->nkeys; ++e)
	{
		uint32 h1, h2;
		uint32 keylen;
		char *key;
		mph->key_source->read(mph->key_source->data, &key, &keylen);
		h1 = hash(czech->hashes[0], key, keylen) % czech->n;
		h2 = hash(czech->hashes[1], key, keylen) % czech->n;
		if (h1 == h2) if (++h2 >= czech->n) h2 = 0;
		if (h1 == h2) 
		{
			if (mph->verbosity) fprintf(stderr, "Self loop for key %e\n", e);
			mph->key_source->dispose(mph->key_source->data, key, keylen);
			return 0;
		}
		DEBUGP("Adding edge: %u -> %u for key %s\n", h1, h2, key);
		mph->key_source->dispose(mph->key_source->data, key, keylen);
		graph_add_edge(czech->graph, h1, h2);
	}
	cycles = graph_is_cyclic(czech->graph);
	if (mph->verbosity && cycles) fprintf(stderr, "Cyclic graph generated\n");
	DEBUGP("Looking for cycles: %u\n", cycles);

	return ! cycles;
}

int czech_mphf_dump(mphf_t *mphf, FILE *fd)
{
	char *buf = NULL;
	uint32 buflen;
	uint32 nbuflen;
	uint32 i;
	uint32 two = htonl(2); //number of hash functions
	czech_mphf_data_t *data = (czech_mphf_data_t *)mphf->data;
	uint32 nn, nm;
	__mphf_dump(mphf, fd);

	fwrite(&two, sizeof(uint32), 1, fd);

	hash_state_dump(data->hashes[0], &buf, &buflen);
	DEBUGP("Dumping hash state with %u bytes to disk\n", buflen);
	nbuflen = htonl(buflen);
	fwrite(&nbuflen, sizeof(uint32), 1, fd);
	fwrite(buf, buflen, 1, fd);
	free(buf);

	hash_state_dump(data->hashes[1], &buf, &buflen);
	DEBUGP("Dumping hash state with %u bytes to disk\n", buflen);
	nbuflen = htonl(buflen);
	fwrite(&nbuflen, sizeof(uint32), 1, fd);
	fwrite(buf, buflen, 1, fd);
	free(buf);

	nn = htonl(data->n);
	fwrite(&nn, sizeof(uint32), 1, fd);
	nm = htonl(data->m);
	fwrite(&nm, sizeof(uint32), 1, fd);
	
	for (i = 0; i < data->n; ++i)
	{
		uint32 ng = htonl(data->g[i]);
		fwrite(&ng, sizeof(uint32), 1, fd);
	}
	#ifdef DEBUG
	fprintf(stderr, "G: ");
	for (i = 0; i < data->n; ++i) fprintf(stderr, "%u ", data->g[i]);
	fprintf(stderr, "\n");
	#endif
	return 1;
}

void czech_mphf_load(FILE *f, mphf_t *mphf)
{
	uint32 nhashes;
	char fbuf[BUFSIZ];
	char *buf = NULL;
	uint32 buflen;
	uint32 i;
	hash_state_t *state;
	czech_mphf_data_t *czech = (czech_mphf_data_t *)malloc(sizeof(czech_mphf_data_t));

	DEBUGP("Loading czech mphf\n");
	mphf->data = czech;
	fread(&nhashes, sizeof(uint32), 1, f);
	nhashes = ntohl(nhashes);
	czech->hashes = (hash_state_t **)malloc(sizeof(hash_state_t *)*(nhashes + 1));
	czech->hashes[nhashes] = NULL;
	DEBUGP("Reading %u hashes\n", nhashes);
	for (i = 0; i < nhashes; ++i)
	{
		hash_state_t *state = NULL;
		fread(&buflen, sizeof(uint32), 1, f);
		buflen = ntohl(buflen);
		DEBUGP("Hash state has %u bytes\n", buflen);
		buf = (char *)malloc(buflen);
		fread(buf, buflen, 1, f);
		state = hash_state_load(buf, buflen);
		czech->hashes[i] = state;
		free(buf);
	}

	DEBUGP("Reading m and n\n");
	fread(&(czech->n), sizeof(uint32), 1, f);	
	czech->n = ntohl(czech->n);
	fread(&(czech->m), sizeof(uint32), 1, f);	
	czech->m = ntohl(czech->m);

	czech->g = (uint32 *)malloc(sizeof(uint32)*czech->n);
	fread(czech->g, czech->n*sizeof(uint32), 1, f);
	for (i = 0; i < czech->n; ++i) czech->g[i] = ntohl(czech->g[i]);
	/*
	#ifdef DEBUG
	fprintf(stderr, "G: ");
	for (i = 0; i < czech->n; ++i) fprintf(stderr, "%u ", czech->g[i]);
	fprintf(stderr, "\n");
	#endif
	*/
	return;
}
		

uint32 czech_mphf_search(mphf_t *mphf, const char *key, uint32 keylen)
{
	czech_mphf_data_t *czech = mphf->data;
	uint32 h1 = hash(czech->hashes[0], key, keylen) % czech->n;
	uint32 h2 = hash(czech->hashes[1], key, keylen) % czech->n;
	DEBUGP("key: %s h1: %u h2: %u\n", key, h1, h2);
	if (h1 == h2 && ++h2 > czech->n) h2 = 0;
	DEBUGP("key: %s g[h1]: %u g[h2]: %u edges: %u\n", key, czech->g[h1], czech->g[h2], czech->m);
	return (czech->g[h1] + czech->g[h2]) % czech->m;
}
void czech_mphf_destroy(mphf_t *mphf)
{
	czech_mphf_data_t *data = (czech_mphf_data_t *)mphf->data;
	free(data->g);	
	hash_state_destroy(data->hashes[0]);
	hash_state_destroy(data->hashes[1]);
	free(data->hashes);
	free(data);
	free(mphf);
}
