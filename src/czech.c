#include "graph.h"
#include "czech.h"
#include "cmph_structs.h"
#include "czech_structs.h"
#include "hash.h"
#include "bitbool.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <netinet/in.h>

//#define DEBUG
#include "debug.h"

/* static const char bitmask[8] = { 1, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7 }; */
/* #define GETBIT(array, i) (array[(i) / 8] & bitmask[(i) % 8]) */
/* #define SETBIT(array, i) (array[(i) / 8] |= bitmask[(i) % 8]) */
/* #define UNSETBIT(array, i) (array[(i) / 8] &= (~(bitmask[(i) % 8]))) */

static int czech_gen_edges(cmph_config_t *mph);
static void czech_traverse(czech_config_data_t *czech, cmph_uint8 *visited, cmph_uint32 v);

czech_config_data_t *czech_config_new(cmph_key_source_t *key_source)
{
	czech_config_data_t *czech = NULL; 	
	czech = (czech_config_data_t *)malloc(sizeof(czech_config_data_t));
	czech->hashfuncs[0] = CMPH_HASH_JENKINS;
	czech->hashfuncs[1] = CMPH_HASH_JENKINS;
	czech->g = NULL;
	czech->graph = NULL;
	czech->hashes = NULL;
	assert(czech);
	return czech;
}
void czech_config_destroy(cmph_config_t *mph)
{
	czech_config_data_t *data = (czech_config_data_t *)mph->data;
	DEBUGP("Destroying algorithm dependent data\n");
	free(data);
}

void czech_config_set_hashfuncs(cmph_config_t *mph, CMPH_HASH *hashfuncs)
{
	czech_config_data_t *czech = (czech_config_data_t *)mph->data;
	CMPH_HASH *hashptr = hashfuncs;
	cmph_uint32 i = 0;
	while(*hashptr != CMPH_HASH_COUNT)
	{
		if (i >= 2) break; //czech only uses two hash functions
		czech->hashfuncs[i] = *hashptr;	
		++i, ++hashptr;
	}
}

cmph_t *czech_new(cmph_config_t *mph, float c)
{
	cmph_t *mphf = NULL;
	czech_data_t *czechf = NULL;

	cmph_uint32 i;
	cmph_uint32 iterations = 20;
	cmph_uint8 *visited = NULL;
	czech_config_data_t *czech = (czech_config_data_t *)mph->data;
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
 	visited = (char *)malloc(czech->n/8 + 1);
	memset(visited, 0, czech->n/8 + 1);
	free(czech->g);
	czech->g = malloc(czech->n * sizeof(cmph_uint32));
	assert(czech->g);
	for (i = 0; i < czech->n; ++i)
	{
	        if (!GETBIT(visited,i))
		{
			czech->g[i] = 0;
			czech_traverse(czech, visited, i);
		}
	}
	graph_destroy(czech->graph);	
	free(visited);
	czech->graph = NULL;

	mphf = (cmph_t *)malloc(sizeof(cmph_t));
	mphf->algo = mph->algo;
	czechf = (czech_data_t *)malloc(sizeof(czech_config_data_t));
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

static void czech_traverse(czech_config_data_t *czech, cmph_uint8 *visited, cmph_uint32 v)
{

	graph_iterator_t it = graph_neighbors_it(czech->graph, v);
	cmph_uint32 neighbor = 0;
	SETBIT(visited,v);
	
	DEBUGP("Visiting vertex %u\n", v);
	while((neighbor = graph_next_neighbor(czech->graph, &it)) != GRAPH_NO_NEIGHBOR)
	{
		DEBUGP("Visiting neighbor %u\n", neighbor);
		if(GETBIT(visited,neighbor)) continue;
		DEBUGP("Visiting neighbor %u\n", neighbor);
		DEBUGP("Visiting edge %u->%u with id %u\n", v, neighbor, graph_edge_id(czech->graph, v, neighbor));
		czech->g[neighbor] = graph_edge_id(czech->graph, v, neighbor) - czech->g[v];
		DEBUGP("g is %u (%u - %u mod %u)\n", czech->g[neighbor], graph_edge_id(czech->graph, v, neighbor), czech->g[v], czech->m);
		czech_traverse(czech, visited, neighbor);
	}
}
		
static int czech_gen_edges(cmph_config_t *mph)
{
	cmph_uint32 e;
	czech_config_data_t *czech = (czech_config_data_t *)mph->data;
	int cycles = 0;

	DEBUGP("Generating edges for %u vertices\n", czech->n);
	graph_clear_edges(czech->graph);	
	mph->key_source->rewind(mph->key_source->data);
	for (e = 0; e < mph->key_source->nkeys; ++e)
	{
		cmph_uint32 h1, h2;
		cmph_uint32 keylen;
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

int czech_dump(cmph_t *mphf, FILE *fd)
{
	char *buf = NULL;
	cmph_uint32 buflen;
	cmph_uint32 nbuflen;
	cmph_uint32 i;
	cmph_uint32 two = htonl(2); //number of hash functions
	czech_data_t *data = (czech_data_t *)mphf->data;
	cmph_uint32 nn, nm;
	__cmph_dump(mphf, fd);

	fwrite(&two, sizeof(cmph_uint32), 1, fd);

	hash_state_dump(data->hashes[0], &buf, &buflen);
	DEBUGP("Dumping hash state with %u bytes to disk\n", buflen);
	nbuflen = htonl(buflen);
	fwrite(&nbuflen, sizeof(cmph_uint32), 1, fd);
	fwrite(buf, buflen, 1, fd);
	free(buf);

	hash_state_dump(data->hashes[1], &buf, &buflen);
	DEBUGP("Dumping hash state with %u bytes to disk\n", buflen);
	nbuflen = htonl(buflen);
	fwrite(&nbuflen, sizeof(cmph_uint32), 1, fd);
	fwrite(buf, buflen, 1, fd);
	free(buf);

	nn = htonl(data->n);
	fwrite(&nn, sizeof(cmph_uint32), 1, fd);
	nm = htonl(data->m);
	fwrite(&nm, sizeof(cmph_uint32), 1, fd);
	
	for (i = 0; i < data->n; ++i)
	{
		cmph_uint32 ng = htonl(data->g[i]);
		fwrite(&ng, sizeof(cmph_uint32), 1, fd);
	}
	#ifdef DEBUG
	fprintf(stderr, "G: ");
	for (i = 0; i < data->n; ++i) fprintf(stderr, "%u ", data->g[i]);
	fprintf(stderr, "\n");
	#endif
	return 1;
}

void czech_load(FILE *f, cmph_t *mphf)
{
	cmph_uint32 nhashes;
	char fbuf[BUFSIZ];
	char *buf = NULL;
	cmph_uint32 buflen;
	cmph_uint32 i;
	hash_state_t *state;
	czech_data_t *czech = (czech_data_t *)malloc(sizeof(czech_data_t));

	DEBUGP("Loading czech mphf\n");
	mphf->data = czech;
	fread(&nhashes, sizeof(cmph_uint32), 1, f);
	nhashes = ntohl(nhashes);
	czech->hashes = (hash_state_t **)malloc(sizeof(hash_state_t *)*(nhashes + 1));
	czech->hashes[nhashes] = NULL;
	DEBUGP("Reading %u hashes\n", nhashes);
	for (i = 0; i < nhashes; ++i)
	{
		hash_state_t *state = NULL;
		fread(&buflen, sizeof(cmph_uint32), 1, f);
		buflen = ntohl(buflen);
		DEBUGP("Hash state has %u bytes\n", buflen);
		buf = (char *)malloc(buflen);
		fread(buf, buflen, 1, f);
		state = hash_state_load(buf, buflen);
		czech->hashes[i] = state;
		free(buf);
	}

	DEBUGP("Reading m and n\n");
	fread(&(czech->n), sizeof(cmph_uint32), 1, f);	
	czech->n = ntohl(czech->n);
	fread(&(czech->m), sizeof(cmph_uint32), 1, f);	
	czech->m = ntohl(czech->m);

	czech->g = (cmph_uint32 *)malloc(sizeof(cmph_uint32)*czech->n);
	fread(czech->g, czech->n*sizeof(cmph_uint32), 1, f);
	for (i = 0; i < czech->n; ++i) czech->g[i] = ntohl(czech->g[i]);
	#ifdef DEBUG
	fprintf(stderr, "G: ");
	for (i = 0; i < czech->n; ++i) fprintf(stderr, "%u ", czech->g[i]);
	fprintf(stderr, "\n");
	#endif
	return;
}
		

cmph_uint32 czech_search(cmph_t *mphf, const char *key, cmph_uint32 keylen)
{
	czech_data_t *czech = mphf->data;
	cmph_uint32 h1 = hash(czech->hashes[0], key, keylen) % czech->n;
	cmph_uint32 h2 = hash(czech->hashes[1], key, keylen) % czech->n;
	DEBUGP("key: %s h1: %u h2: %u\n", key, h1, h2);
	if (h1 == h2 && ++h2 > czech->n) h2 = 0;
	DEBUGP("key: %s g[h1]: %u g[h2]: %u edges: %u\n", key, czech->g[h1], czech->g[h2], czech->m);
	return (czech->g[h1] + czech->g[h2]) % czech->m;
}
void czech_destroy(cmph_t *mphf)
{
	czech_data_t *data = (czech_data_t *)mphf->data;
	free(data->g);	
	hash_state_destroy(data->hashes[0]);
	hash_state_destroy(data->hashes[1]);
	free(data->hashes);
	free(data);
	free(mphf);
}
