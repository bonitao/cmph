#include "bmz.h"
#include "cmph_structs.h"
#include "bmz_structs.h"
#include "hash.h"
#include "vqueue.h"

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <netinet/in.h>

//#define DEBUG
#include "debug.h"

static uint32 UNDEFINED = UINT_MAX;

static const char bitmask[8] = { 1, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7 };
#define GETBIT(array, i) (array[(i) / 8] & bitmask[(i) % 8])
#define SETBIT(array, i) (array[(i) / 8] |= bitmask[(i) % 8])
#define UNSETBIT(array, i) (array[(i) / 8] &= (~(bitmask[(i) % 8])))

static int bmz_gen_edges(mph_t *mph);
static void bmz_traverse_critical_nodes(bmz_mph_data_t *bmz, uint32 v, uint32 * biggest_g_value, uint32 * biggest_edge_value, uint8 * used_edges);
static void bmz_traverse_non_critical_nodes(bmz_mph_data_t *bmz, uint8 * used_edges);


mph_t *bmz_mph_new(key_source_t *key_source)
{
	mph_t *mph = NULL;
	bmz_mph_data_t *bmz = NULL; 	
	mph = __mph_new(MPH_BMZ, key_source);
	if (mph == NULL) return NULL;
	bmz = (bmz_mph_data_t *)malloc(sizeof(bmz_mph_data_t));
	if (bmz == NULL)
	{
		__mph_destroy(mph);
		return NULL;
	}
	bmz->hashfuncs[0] = HASH_JENKINS;
	bmz->hashfuncs[1] = HASH_JENKINS;
	bmz->g = NULL;
	bmz->graph = NULL;
	bmz->hashes = NULL;
	mph->data = bmz;
	assert(mph->data);
	return mph;
}
void bmz_mph_destroy(mph_t *mph)
{
	bmz_mph_data_t *data = (bmz_mph_data_t *)mph->data;
	DEBUGP("Destroying algorithm dependent data\n");
	free(data);
	__mph_destroy(mph);
}

void bmz_mph_set_hashfuncs(mph_t *mph, CMPH_HASH *hashfuncs)
{
	bmz_mph_data_t *bmz = (bmz_mph_data_t *)mph->data;
	CMPH_HASH *hashptr = hashfuncs;
	uint32 i = 0;
	while(*hashptr != HASH_COUNT)
	{
		if (i >= 2) break; //bmz only uses two hash functions
		bmz->hashfuncs[i] = *hashptr;	
		++i, ++hashptr;
	}
}

mphf_t *bmz_mph_create(mph_t *mph, float bmz_c)
{
	mphf_t *mphf = NULL;
	bmz_mphf_data_t *bmzf = NULL;

	uint32 i;
	uint32 iterations = 10;
	uint8 *used_edges = NULL;
	uint32 biggest_g_value = 0;
	uint32 biggest_edge_value = 1;	
	DEBUGP("bmz_c: %f\n", bmz_c);
	bmz_mph_data_t *bmz = (bmz_mph_data_t *)mph->data;
	bmz->m = mph->key_source->nkeys;	
	bmz->n = ceil(bmz_c * mph->key_source->nkeys);	
	DEBUGP("m (edges): %u n (vertices): %u bmz_c: %f\n", bmz->m, bmz->n, bmz_c);
	bmz->graph = graph_new(bmz->n, bmz->m);
	DEBUGP("Created graph\n");

	bmz->hashes = (hash_state_t **)malloc(sizeof(hash_state_t *)*3);
	for(i = 0; i < 3; ++i) bmz->hashes[i] = NULL;

	// Mapping step
	if (mph->verbosity)
	{
		fprintf(stderr, "Entering mapping step for mph creation of %u keys with graph sized %u\n", bmz->m, bmz->n);
	}
	while(1)
	{
		int ok;
		DEBUGP("hash function 1\n");
		bmz->hashes[0] = hash_state_new(bmz->hashfuncs[0], bmz->n);
		DEBUGP("hash function 2\n");
		bmz->hashes[1] = hash_state_new(bmz->hashfuncs[1], bmz->n);
		DEBUGP("Generating edges\n");
		ok = bmz_gen_edges(mph);
		if (!ok)
		{
			--iterations;
			hash_state_destroy(bmz->hashes[0]);
			bmz->hashes[0] = NULL;
			hash_state_destroy(bmz->hashes[1]);
			bmz->hashes[1] = NULL;
			DEBUGP("%u iterations remaining\n", iterations);
			if (mph->verbosity)
			{
				fprintf(stderr, "simple graph creation failure - %u iterations remaining\n", iterations);
			}
			if (iterations == 0) break;
		} 
		else break;	
	}
	if (iterations == 0)
	{
		graph_destroy(bmz->graph);	
		return NULL;
	}

	// Ordering step
	if (mph->verbosity)
	{
		fprintf(stderr, "Starting ordering step\n");
	}

	graph_obtain_critical_nodes(bmz->graph);

	// Searching step
	if (mph->verbosity)
	{
		fprintf(stderr, "Starting Searching step.\n");
		fprintf(stderr, "\tTraversing critical vertices.\n");
	}
	DEBUGP("Searching step\n");

	used_edges = (uint8 *)malloc((bmz->m*sizeof(uint8))/8 + 1);
	memset(used_edges, 0, bmz->m/8 + 1);
	free(bmz->g);
	bmz->g = malloc(bmz->n * sizeof(uint32));
	assert(bmz->g);
	for (i = 0; i < bmz->n; ++i) bmz->g[i] =  UNDEFINED;

	for (i = 0; i < bmz->n; ++i) // critical nodes
	{
		if (graph_node_is_critical(bmz->graph, i) && (bmz->g[i] == UNDEFINED))
		{
			bmz_traverse_critical_nodes(bmz, i, &biggest_g_value, &biggest_edge_value, used_edges);
		}
	}
	if (mph->verbosity)
	{
		fprintf(stderr, "\tTraversing non critical vertices.\n");
	}

	bmz_traverse_non_critical_nodes(bmz, used_edges); // non_critical_nodes
	graph_destroy(bmz->graph);	
	free(used_edges);
	bmz->graph = NULL;

	mphf = (mphf_t *)malloc(sizeof(mphf_t));
	mphf->algo = mph->algo;
	bmzf = (bmz_mphf_data_t *)malloc(sizeof(bmz_mph_data_t));
	bmzf->g = bmz->g;
	bmz->g = NULL; //transfer memory ownership
	bmzf->hashes = bmz->hashes;
	bmz->hashes = NULL; //transfer memory ownership
	bmzf->n = bmz->n;
	bmzf->m = bmz->m;
	mphf->data = bmzf;
	mphf->size = bmz->m;
	DEBUGP("Successfully generated minimal perfect hash\n");
	if (mph->verbosity)
	{
		fprintf(stderr, "Successfully generated minimal perfect hash function\n");
	}
	return mphf;
}

static void bmz_traverse_critical_nodes(bmz_mph_data_t *bmz, uint32 v, uint32 * biggest_g_value, uint32 * biggest_edge_value, uint8 * used_edges)
{
        uint32 next_g;
	uint32 u;   /* Auxiliary vertex */
	uint32 lav; /* lookahead vertex */
	uint8 collision;
	vqueue_t * q = vqueue_new((uint32)(0.5*graph_ncritical_nodes(bmz->graph)));
	graph_iterator_t it, it1;

	DEBUGP("Labelling critical vertices\n");
	bmz->g[v] = (uint32)ceil ((double)(*biggest_edge_value)/2) - 1;
	next_g    = (uint32)floor((double)(*biggest_edge_value/2)); /* next_g is incremented in the do..while statement*/
	*biggest_g_value = next_g;
	vqueue_insert(q, v);
	while(!vqueue_is_empty(q))
	{
		v = vqueue_remove(q);
		it = graph_neighbors_it(bmz->graph, v);		
		while ((u = graph_next_neighbor(bmz->graph, &it)) != GRAPH_NO_NEIGHBOR)
		{		        
		        if (graph_node_is_critical(bmz->graph, u) && (bmz->g[u] == UNDEFINED))
			{
			        collision = 1;
			        while(collision) // lookahead to resolve collisions
				{
 				        next_g = *biggest_g_value + 1; 
					it1 = graph_neighbors_it(bmz->graph, u);
					collision = 0;
					while((lav = graph_next_neighbor(bmz->graph, &it1)) != GRAPH_NO_NEIGHBOR)
					{
					        if (graph_node_is_critical(bmz->graph, lav) && (bmz->g[lav] != UNDEFINED))
						{
   						        assert(next_g + bmz->g[lav] < bmz->m);
							if (GETBIT(used_edges, next_g + bmz->g[lav])) 
							{
							        collision = 1;
								break;
							}
						}
					}
 					if (next_g > *biggest_g_value) *biggest_g_value = next_g;
				}		
				// Marking used edges...
				it1 = graph_neighbors_it(bmz->graph, u);
				while((lav = graph_next_neighbor(bmz->graph, &it1)) != GRAPH_NO_NEIGHBOR)
				{
				        if (graph_node_is_critical(bmz->graph, lav) && (bmz->g[lav] != UNDEFINED))
					{
                   			        SETBIT(used_edges,next_g + bmz->g[lav]);
						if(next_g + bmz->g[lav] > *biggest_edge_value) *biggest_edge_value = next_g + bmz->g[lav];
					}
				}
				bmz->g[u] = next_g; // Labelling vertex u.
			        vqueue_insert(q, u);
			}			
	        }
		 
	}
	vqueue_destroy(q);
  
}

static uint32 next_unused_edge(bmz_mph_data_t *bmz, uint8 * used_edges, uint32 unused_edge_index)
{
       while(1)
       {
		assert(unused_edge_index < bmz->m);
		if(GETBIT(used_edges, unused_edge_index)) unused_edge_index ++;
		else break;
       }
       return unused_edge_index;
}

static void bmz_traverse(bmz_mph_data_t *bmz, uint8 * used_edges, uint32 v, uint32 * unused_edge_index)
{
	graph_iterator_t it = graph_neighbors_it(bmz->graph, v);
	uint32 neighbor = 0;
	while((neighbor = graph_next_neighbor(bmz->graph, &it)) != GRAPH_NO_NEIGHBOR)
	{
		DEBUGP("Visiting neighbor %u\n", neighbor);
		if(bmz->g[neighbor] != UNDEFINED) continue;
		*unused_edge_index = next_unused_edge(bmz, used_edges, *unused_edge_index);
		bmz->g[neighbor] = *unused_edge_index - bmz->g[v];
		(*unused_edge_index)++;
		bmz_traverse(bmz, used_edges, neighbor, unused_edge_index);
		
	}  
}

static void bmz_traverse_non_critical_nodes(bmz_mph_data_t *bmz, uint8 * used_edges)
{

	uint32 i, v1, v2, unused_edge_index = 0;
	DEBUGP("Labelling non critical vertices\n");
	for(i = 0; i < bmz->m; i++)
	{
	        v1 = graph_vertex_id(bmz->graph, i, 0);
		v2 = graph_vertex_id(bmz->graph, i, 1);
		if((bmz->g[v1] != UNDEFINED && bmz->g[v2] != UNDEFINED) || (bmz->g[v1] == UNDEFINED && bmz->g[v2] == UNDEFINED)) continue;				  	
		if(bmz->g[v1] != UNDEFINED) bmz_traverse(bmz, used_edges, v1, &unused_edge_index);
	        else bmz_traverse(bmz, used_edges, v2, &unused_edge_index);
	}

	for(i = 0; i < bmz->n; i++)
	{
		if(bmz->g[i] == UNDEFINED)
		{ 	                        
		        bmz->g[i] = 0;
			bmz_traverse(bmz, used_edges, i, &unused_edge_index);
		}
	}

}
		
static int bmz_gen_edges(mph_t *mph)
{
	uint32 e;
	bmz_mph_data_t *bmz = (bmz_mph_data_t *)mph->data;
	uint8 multiple_edges = 0;

	DEBUGP("Generating edges for %u vertices\n", bmz->n);
	graph_clear_edges(bmz->graph);	
	mph->key_source->rewind(mph->key_source->data);
	for (e = 0; e < mph->key_source->nkeys; ++e)
	{
		uint32 h1, h2;
		uint32 keylen;
		char *key;
		mph->key_source->read(mph->key_source->data, &key, &keylen);
		h1 = hash(bmz->hashes[0], key, keylen) % bmz->n;
		h2 = hash(bmz->hashes[1], key, keylen) % bmz->n;
		if (h1 == h2) if (++h2 >= bmz->n) h2 = 0;
		if (h1 == h2) 
		{
			if (mph->verbosity) fprintf(stderr, "Self loop for key %e\n", e);
			mph->key_source->dispose(mph->key_source->data, key, keylen);
			return 0;
		}
		DEBUGP("Adding edge: %u -> %u for key %s\n", h1, h2, key);
		mph->key_source->dispose(mph->key_source->data, key, keylen);
		multiple_edges = graph_contains_edge(bmz->graph, h1, h2);
		if (mph->verbosity && multiple_edges) fprintf(stderr, "A non simple graph was generated\n");
		if (multiple_edges) return 0; // checking multiple edge restriction.
		graph_add_edge(bmz->graph, h1, h2);
	}
	return !multiple_edges;
}

int bmz_mphf_dump(mphf_t *mphf, FILE *fd)
{
	char *buf = NULL;
	uint32 buflen;
	uint32 nbuflen;
	uint32 i;
	uint32 two = htonl(2); //number of hash functions
	bmz_mphf_data_t *data = (bmz_mphf_data_t *)mphf->data;
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

void bmz_mphf_load(FILE *f, mphf_t *mphf)
{
	uint32 nhashes;
	char fbuf[BUFSIZ];
	char *buf = NULL;
	uint32 buflen;
	uint32 i;
	hash_state_t *state;
	bmz_mphf_data_t *bmz = (bmz_mphf_data_t *)malloc(sizeof(bmz_mphf_data_t));

	DEBUGP("Loading bmz mphf\n");
	mphf->data = bmz;
	fread(&nhashes, sizeof(uint32), 1, f);
	nhashes = ntohl(nhashes);
	bmz->hashes = (hash_state_t **)malloc(sizeof(hash_state_t *)*(nhashes + 1));
	bmz->hashes[nhashes] = NULL;
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
		bmz->hashes[i] = state;
		free(buf);
	}

	DEBUGP("Reading m and n\n");
	fread(&(bmz->n), sizeof(uint32), 1, f);	
	bmz->n = ntohl(bmz->n);
	fread(&(bmz->m), sizeof(uint32), 1, f);	
	bmz->m = ntohl(bmz->m);

	bmz->g = (uint32 *)malloc(sizeof(uint32)*bmz->n);
	fread(bmz->g, bmz->n*sizeof(uint32), 1, f);
	for (i = 0; i < bmz->n; ++i) bmz->g[i] = ntohl(bmz->g[i]);
	#ifdef DEBUG
	fprintf(stderr, "G: ");
	for (i = 0; i < bmz->n; ++i) fprintf(stderr, "%u ", bmz->g[i]);
	fprintf(stderr, "\n");
	#endif
	return;
}
		

uint32 bmz_mphf_search(mphf_t *mphf, const char *key, uint32 keylen)
{
	bmz_mphf_data_t *bmz = mphf->data;
	uint32 h1 = hash(bmz->hashes[0], key, keylen) % bmz->n;
	uint32 h2 = hash(bmz->hashes[1], key, keylen) % bmz->n;
	DEBUGP("key: %s h1: %u h2: %u\n", key, h1, h2);
	if (h1 == h2 && ++h2 > bmz->n) h2 = 0;
	DEBUGP("key: %s g[h1]: %u g[h2]: %u edges: %u\n", key, bmz->g[h1], bmz->g[h2], bmz->m);
	return bmz->g[h1] + bmz->g[h2];
}
void bmz_mphf_destroy(mphf_t *mphf)
{
	bmz_mphf_data_t *data = (bmz_mphf_data_t *)mphf->data;
	free(data->g);	
	hash_state_destroy(data->hashes[0]);
	hash_state_destroy(data->hashes[1]);
	free(data->hashes);
	free(data);
	free(mphf);
}
