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

//#define DEBUG
#include "debug.h"

//static cmph_uint32 UNDEFINED = UINT_MAX;

static const char bitmask[8] = { 1, 1 << 1, 1 << 2, 1 << 3, 1 << 4, 1 << 5, 1 << 6, 1 << 7 };
#define GETBIT(array, i) (array[(i) / 8] & bitmask[(i) % 8])
#define SETBIT(array, i) (array[(i) / 8] |= bitmask[(i) % 8])
#define UNSETBIT(array, i) (array[(i) / 8] &= (~(bitmask[(i) % 8])))

static int bmz_gen_edges(cmph_mph_t *mph);
static cmph_uint8 bmz_traverse_critical_nodes(cmph_bmz_mph_data_t *bmz, cmph_uint32 v, cmph_uint32 * biggest_g_value, cmph_uint32 * biggest_edge_value, cmph_uint8 * used_edges, cmph_uint8 * visited);
static cmph_uint8 bmz_traverse_critical_nodes_heuristic(cmph_bmz_mph_data_t *bmz, cmph_uint32 v, cmph_uint32 * biggest_g_value, cmph_uint32 * biggest_edge_value, cmph_uint8 * used_edges, cmph_uint8 * visited);
static void bmz_traverse_non_critical_nodes(cmph_bmz_mph_data_t *bmz, cmph_uint8 * used_edges, cmph_uint8 * visited);


cmph_mph_t *cmph_bmz_mph_new(cmph_key_source_t *key_source)
{
	cmph_mph_t *mph = NULL;
	cmph_bmz_mph_data_t *bmz = NULL; 	
	mph = cmph__mph_new(CMPH_BMZ, key_source);
	if (mph == NULL) return NULL;
	bmz = (cmph_bmz_mph_data_t *)malloc(sizeof(cmph_bmz_mph_data_t));
	if (bmz == NULL)
	{
		cmph__mph_destroy(mph);
		return NULL;
	}
	bmz->hashfuncs[0] = CMPH_HASH_JENKINS;
	bmz->hashfuncs[1] = CMPH_HASH_JENKINS;
	bmz->g = NULL;
	bmz->graph = NULL;
	bmz->hashes = NULL;
	mph->data = bmz;
	assert(mph->data);
	return mph;
}
void cmph_bmz_mph_destroy(cmph_mph_t *mph)
{
	cmph_bmz_mph_data_t *data = (cmph_bmz_mph_data_t *)mph->data;
	DEBUGP("Destroying algorithm dependent data\n");
	free(data);
	cmph__mph_destroy(mph);
}

void cmph_bmz_mph_set_hashfuncs(cmph_mph_t *mph, CMPH_HASH *hashfuncs)
{
	cmph_bmz_mph_data_t *bmz = (cmph_bmz_mph_data_t *)mph->data;
	CMPH_HASH *hashptr = hashfuncs;
	cmph_uint32 i = 0;
	while(*hashptr != CMPH_HASH_COUNT)
	{
		if (i >= 2) break; //bmz only uses two hash functions
		bmz->hashfuncs[i] = *hashptr;	
		++i, ++hashptr;
	}
}

cmph_mphf_t *cmph_bmz_mph_create(cmph_mph_t *mph, float bmz_c)
{
	cmph_mphf_t *mphf = NULL;
	cmph_bmz_mphf_data_t *bmzf = NULL;
	cmph_uint32 i;
	cmph_uint32 iterations;
	cmph_uint32 iterations_map = 20;
	cmph_uint8 *used_edges = NULL;
	cmph_uint8 restart_mapping = 0;
	cmph_uint8 * visited = NULL;
	
	DEBUGP("bmz_c: %f\n", bmz_c);
	cmph_bmz_mph_data_t *bmz = (cmph_bmz_mph_data_t *)mph->data;
	bmz->m = mph->key_source->nkeys;	
	bmz->n = ceil(bmz_c * mph->key_source->nkeys);	
	DEBUGP("m (edges): %u n (vertices): %u bmz_c: %f\n", bmz->m, bmz->n, bmz_c);
	bmz->graph = cmph_graph_new(bmz->n, bmz->m);
	DEBUGP("Created graph\n");

	bmz->hashes = (cmph_hash_state_t **)malloc(sizeof(cmph_hash_state_t *)*3);
	for(i = 0; i < 3; ++i) bmz->hashes[i] = NULL;

	do
	{
	  // Mapping step
	  cmph_uint32 biggest_g_value = 0;
	  cmph_uint32 biggest_edge_value = 1;	
	  iterations = 20;
	  if (mph->verbosity)
	  {
		fprintf(stderr, "Entering mapping step for mph creation of %u keys with graph sized %u\n", bmz->m, bmz->n);
	  }
	  while(1)
	  {
		int ok;
		DEBUGP("hash function 1\n");
		bmz->hashes[0] = cmph_hash_state_new(bmz->hashfuncs[0], bmz->n);
		DEBUGP("hash function 2\n");
		bmz->hashes[1] = cmph_hash_state_new(bmz->hashfuncs[1], bmz->n);
		DEBUGP("Generating edges\n");
		ok = bmz_gen_edges(mph);
		if (!ok)
		{
			--iterations;
			cmph_hash_state_destroy(bmz->hashes[0]);
			bmz->hashes[0] = NULL;
			cmph_hash_state_destroy(bmz->hashes[1]);
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
		cmph_graph_destroy(bmz->graph);	
		return NULL;
	  }

	  // Ordering step
	  if (mph->verbosity)
	  {
		fprintf(stderr, "Starting ordering step\n");
	  }

	  cmph_graph_obtain_critical_nodes(bmz->graph);

	  // Searching step
	  if (mph->verbosity)
	  {
		fprintf(stderr, "Starting Searching step.\n");
		fprintf(stderr, "\tTraversing critical vertices.\n");
	  }
	  DEBUGP("Searching step\n");
	  visited = (char *)malloc(bmz->n/8 + 1);
	  memset(visited, 0, bmz->n/8 + 1);
	  used_edges = (cmph_uint8 *)malloc(bmz->m/8 + 1);
	  memset(used_edges, 0, bmz->m/8 + 1);
	  free(bmz->g);
	  bmz->g = malloc(bmz->n * sizeof(cmph_uint32));
	  assert(bmz->g);
	  for (i = 0; i < bmz->n; ++i) // critical nodes
	  {
                if (cmph_graph_node_is_critical(bmz->graph, i) && (!GETBIT(visited,i)))
		{
		  if(bmz_c > 1.14) restart_mapping = bmz_traverse_critical_nodes(bmz, i, &biggest_g_value, &biggest_edge_value, used_edges, visited);
		  else restart_mapping = bmz_traverse_critical_nodes_heuristic(bmz, i, &biggest_g_value, &biggest_edge_value, used_edges, visited);
		  if(restart_mapping) break;
		}
	  }
	  if(!restart_mapping)
	  {
	        if (mph->verbosity)
	        {
		  fprintf(stderr, "\tTraversing non critical vertices.\n");
		}
		bmz_traverse_non_critical_nodes(bmz, used_edges, visited); // non_critical_nodes
	  }
	  else 
	  {
 	        iterations_map--;
		if (mph->verbosity) fprintf(stderr, "Restarting mapping step. %u iterations remaining.\n", iterations_map);
	  }    
	  free(used_edges);
	  free(visited);
        }while(restart_mapping && iterations_map > 0);
	cmph_graph_destroy(bmz->graph);	
	bmz->graph = NULL;
	if (iterations_map == 0) return NULL;
	mphf = (cmph_mphf_t *)malloc(sizeof(cmph_mphf_t));
	mphf->algo = mph->algo;
	bmzf = (cmph_bmz_mphf_data_t *)malloc(sizeof(cmph_bmz_mph_data_t));
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

static cmph_uint8 bmz_traverse_critical_nodes(cmph_bmz_mph_data_t *bmz, cmph_uint32 v, cmph_uint32 * biggest_g_value, cmph_uint32 * biggest_edge_value, cmph_uint8 * used_edges, cmph_uint8 * visited)
{
        cmph_uint32 next_g;
	cmph_uint32 u;   /* Auxiliary vertex */
	cmph_uint32 lav; /* lookahead vertex */
	cmph_uint8 collision;
	cmph_vqueue_t * q = cmph_vqueue_new((cmph_uint32)(0.5*cmph_graph_ncritical_nodes(bmz->graph)) + 1);
	cmph_graph_iterator_t it, it1;

	DEBUGP("Labelling critical vertices\n");
	bmz->g[v] = (cmph_uint32)ceil ((double)(*biggest_edge_value)/2) - 1;
	SETBIT(visited, v);
	next_g    = (cmph_uint32)floor((double)(*biggest_edge_value/2)); /* next_g is incremented in the do..while statement*/
	cmph_vqueue_insert(q, v);
	while(!cmph_vqueue_is_empty(q))
	{
		v = cmph_vqueue_remove(q);
		it = cmph_graph_neighbors_it(bmz->graph, v);		
		while ((u = cmph_graph_next_neighbor(bmz->graph, &it)) != CMPH_GRAPH_NO_NEIGHBOR)
		{		        
               	        if (cmph_graph_node_is_critical(bmz->graph, u) && (!GETBIT(visited,u)))
			{
			        collision = 1;
			        while(collision) // lookahead to resolve collisions
				{
 				        next_g = *biggest_g_value + 1; 
					it1 = cmph_graph_neighbors_it(bmz->graph, u);
					collision = 0;
					while((lav = cmph_graph_next_neighbor(bmz->graph, &it1)) != CMPH_GRAPH_NO_NEIGHBOR)
					{
  					        if (cmph_graph_node_is_critical(bmz->graph, lav) && GETBIT(visited,lav))
						{
   						        if(next_g + bmz->g[lav] >= bmz->m)
							{
							        cmph_vqueue_destroy(q);
								return 1; // restart mapping step.
							}
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
				it1 = cmph_graph_neighbors_it(bmz->graph, u);
				while((lav = cmph_graph_next_neighbor(bmz->graph, &it1)) != CMPH_GRAPH_NO_NEIGHBOR)
				{
                 		        if (cmph_graph_node_is_critical(bmz->graph, lav) && GETBIT(visited, lav))
					{
                   			        SETBIT(used_edges,next_g + bmz->g[lav]);
						if(next_g + bmz->g[lav] > *biggest_edge_value) *biggest_edge_value = next_g + bmz->g[lav];
					}
				}
				bmz->g[u] = next_g; // Labelling vertex u.
				SETBIT(visited,u);
			        cmph_vqueue_insert(q, u);
			}			
	        }
		 
	}
	cmph_vqueue_destroy(q);
	return 0;
}

static cmph_uint8 bmz_traverse_critical_nodes_heuristic(cmph_bmz_mph_data_t *bmz, cmph_uint32 v, cmph_uint32 * biggest_g_value, cmph_uint32 * biggest_edge_value, cmph_uint8 * used_edges, cmph_uint8 * visited)
{
        cmph_uint32 next_g;
	cmph_uint32 u;   /* Auxiliary vertex */
	cmph_uint32 lav; /* lookahead vertex */
	cmph_uint8 collision;
	cmph_uint32 * unused_g_values = NULL;
	cmph_uint32 unused_g_values_capacity = 0;
	cmph_uint32 nunused_g_values = 0;
	cmph_vqueue_t * q = cmph_vqueue_new((cmph_uint32)(0.5*cmph_graph_ncritical_nodes(bmz->graph))+1);
	cmph_graph_iterator_t it, it1;

	DEBUGP("Labelling critical vertices\n");
	bmz->g[v] = (cmph_uint32)ceil ((double)(*biggest_edge_value)/2) - 1;
	SETBIT(visited, v);
	next_g    = (cmph_uint32)floor((double)(*biggest_edge_value/2)); /* next_g is incremented in the do..while statement*/
	cmph_vqueue_insert(q, v);
	while(!cmph_vqueue_is_empty(q))
	{
		v = cmph_vqueue_remove(q);
		it = cmph_graph_neighbors_it(bmz->graph, v);		
		while ((u = cmph_graph_next_neighbor(bmz->graph, &it)) != CMPH_GRAPH_NO_NEIGHBOR)
		{		        
               	        if (cmph_graph_node_is_critical(bmz->graph, u) && (!GETBIT(visited,u)))
			{
			        cmph_uint32 next_g_index = 0;
			        collision = 1;
			        while(collision) // lookahead to resolve collisions
				{
				        if (next_g_index < nunused_g_values) 
					{
					        next_g = unused_g_values[next_g_index++];						
					}
					else    
					{
					        next_g = *biggest_g_value + 1; 
						next_g_index = UINT_MAX;
					}
					it1 = cmph_graph_neighbors_it(bmz->graph, u);
					collision = 0;
					while((lav = cmph_graph_next_neighbor(bmz->graph, &it1)) != CMPH_GRAPH_NO_NEIGHBOR)
					{
  					        if (cmph_graph_node_is_critical(bmz->graph, lav) && GETBIT(visited,lav))
						{
   						        if(next_g + bmz->g[lav] >= bmz->m)
							{
							        cmph_vqueue_destroy(q);
								free(unused_g_values);
								return 1; // restart mapping step.
							}
							if (GETBIT(used_edges, next_g + bmz->g[lav])) 
							{
							        collision = 1;
								break;
							}
						}
					}
					if(collision && (next_g > *biggest_g_value)) // saving the current g value stored in next_g.
					{
					        if(nunused_g_values == unused_g_values_capacity)
						{
   						        unused_g_values = realloc(unused_g_values, (unused_g_values_capacity + BUFSIZ)*sizeof(cmph_uint32));
						        unused_g_values_capacity += BUFSIZ;  							
						} 
						unused_g_values[nunused_g_values++] = next_g;							

					}
 					if (next_g > *biggest_g_value) *biggest_g_value = next_g;
				}	
				next_g_index--;
				if (next_g_index < nunused_g_values) unused_g_values[next_g_index] = unused_g_values[--nunused_g_values];

				// Marking used edges...
				it1 = cmph_graph_neighbors_it(bmz->graph, u);
				while((lav = cmph_graph_next_neighbor(bmz->graph, &it1)) != CMPH_GRAPH_NO_NEIGHBOR)
				{
                 		        if (cmph_graph_node_is_critical(bmz->graph, lav) && GETBIT(visited, lav))
					{
                   			        SETBIT(used_edges,next_g + bmz->g[lav]);
						if(next_g + bmz->g[lav] > *biggest_edge_value) *biggest_edge_value = next_g + bmz->g[lav];
					}
				}
				bmz->g[u] = next_g; // Labelling vertex u.
				SETBIT(visited, u);
			        cmph_vqueue_insert(q, u);
			}			
	        }
		 
	}
	cmph_vqueue_destroy(q);
	free(unused_g_values);
	return 0;  
}

static cmph_uint32 next_unused_edge(cmph_bmz_mph_data_t *bmz, cmph_uint8 * used_edges, cmph_uint32 unused_edge_index)
{
       while(1)
       {
		assert(unused_edge_index < bmz->m);
		if(GETBIT(used_edges, unused_edge_index)) unused_edge_index ++;
		else break;
       }
       return unused_edge_index;
}

static void bmz_traverse(cmph_bmz_mph_data_t *bmz, cmph_uint8 * used_edges, cmph_uint32 v, cmph_uint32 * unused_edge_index, cmph_uint8 * visited)
{
	cmph_graph_iterator_t it = cmph_graph_neighbors_it(bmz->graph, v);
	cmph_uint32 neighbor = 0;
	while((neighbor = cmph_graph_next_neighbor(bmz->graph, &it)) != CMPH_GRAPH_NO_NEIGHBOR)
	{
     	        if(GETBIT(visited,neighbor)) continue;
		DEBUGP("Visiting neighbor %u\n", neighbor);
		*unused_edge_index = next_unused_edge(bmz, used_edges, *unused_edge_index);
		bmz->g[neighbor] = *unused_edge_index - bmz->g[v];
		SETBIT(visited, neighbor);
		(*unused_edge_index)++;
		bmz_traverse(bmz, used_edges, neighbor, unused_edge_index, visited);
		
	}  
}

static void bmz_traverse_non_critical_nodes(cmph_bmz_mph_data_t *bmz, cmph_uint8 * used_edges, cmph_uint8 * visited)
{

	cmph_uint32 i, v1, v2, unused_edge_index = 0;
	DEBUGP("Labelling non critical vertices\n");
	for(i = 0; i < bmz->m; i++)
	{
	        v1 = cmph_graph_vertex_id(bmz->graph, i, 0);
		v2 = cmph_graph_vertex_id(bmz->graph, i, 1);
		if((GETBIT(visited,v1) && GETBIT(visited,v2)) || (!GETBIT(visited,v1) && !GETBIT(visited,v2))) continue;				  	
		if(GETBIT(visited,v1)) bmz_traverse(bmz, used_edges, v1, &unused_edge_index, visited);
	        else bmz_traverse(bmz, used_edges, v2, &unused_edge_index, visited);

	}

	for(i = 0; i < bmz->n; i++)
	{
		if(!GETBIT(visited,i))
		{ 	                        
		        bmz->g[i] = 0;
			SETBIT(visited, i);
			bmz_traverse(bmz, used_edges, i, &unused_edge_index, visited);
		}
	}

}
		
static int bmz_gen_edges(cmph_mph_t *mph)
{
	cmph_uint32 e;
	cmph_bmz_mph_data_t *bmz = (cmph_bmz_mph_data_t *)mph->data;
	cmph_uint8 multiple_edges = 0;

	DEBUGP("Generating edges for %u vertices\n", bmz->n);
	cmph_graph_clear_edges(bmz->graph);	
	mph->key_source->rewind(mph->key_source->data);
	for (e = 0; e < mph->key_source->nkeys; ++e)
	{
		cmph_uint32 h1, h2;
		cmph_uint32 keylen;
		char *key;
		mph->key_source->read(mph->key_source->data, &key, &keylen);
		h1 = cmph_hash(bmz->hashes[0], key, keylen) % bmz->n;
		h2 = cmph_hash(bmz->hashes[1], key, keylen) % bmz->n;
		if (h1 == h2) if (++h2 >= bmz->n) h2 = 0;
		if (h1 == h2) 
		{
			if (mph->verbosity) fprintf(stderr, "Self loop for key %u\n", e);
			mph->key_source->dispose(mph->key_source->data, key, keylen);
			return 0;
		}
		DEBUGP("Adding edge: %u -> %u for key %s\n", h1, h2, key);
		mph->key_source->dispose(mph->key_source->data, key, keylen);
		multiple_edges = cmph_graph_contains_edge(bmz->graph, h1, h2);
		if (mph->verbosity && multiple_edges) fprintf(stderr, "A non simple graph was generated\n");
		if (multiple_edges) return 0; // checking multiple edge restriction.
		cmph_graph_add_edge(bmz->graph, h1, h2);
	}
	return !multiple_edges;
}

int cmph_bmz_mphf_dump(cmph_mphf_t *mphf, FILE *fd)
{
	char *buf = NULL;
	cmph_uint32 buflen;
	cmph_uint32 nbuflen;
	cmph_uint32 i;
	cmph_uint32 two = 2; //number of hash functions
	cmph_bmz_mphf_data_t *data = (cmph_bmz_mphf_data_t *)mphf->data;
	cmph_uint32 nn, nm;
	cmph__mphf_dump(mphf, fd);

	fwrite(&two, sizeof(cmph_uint32), 1, fd);

	cmph_hash_state_dump(data->hashes[0], &buf, &buflen);
	DEBUGP("Dumping hash state with %u bytes to disk\n", buflen);
	fwrite(&buflen, sizeof(cmph_uint32), 1, fd);
	fwrite(buf, buflen, 1, fd);
	free(buf);

	cmph_hash_state_dump(data->hashes[1], &buf, &buflen);
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
	return 1;
}

void cmph_bmz_mphf_load(FILE *f, cmph_mphf_t *mphf)
{
	cmph_uint32 nhashes;
	char *buf = NULL;
	cmph_uint32 buflen;
	cmph_uint32 i;
	cmph_bmz_mphf_data_t *bmz = (cmph_bmz_mphf_data_t *)malloc(sizeof(cmph_bmz_mphf_data_t));

	DEBUGP("Loading bmz mphf\n");
	mphf->data = bmz;
	fread(&nhashes, sizeof(cmph_uint32), 1, f);
	bmz->hashes = (cmph_hash_state_t **)malloc(sizeof(cmph_hash_state_t *)*(nhashes + 1));
	bmz->hashes[nhashes] = NULL;
	DEBUGP("Reading %u hashes\n", nhashes);
	for (i = 0; i < nhashes; ++i)
	{
		cmph_hash_state_t *state = NULL;
		fread(&buflen, sizeof(cmph_uint32), 1, f);
		DEBUGP("Hash state has %u bytes\n", buflen);
		buf = (char *)malloc(buflen);
		fread(buf, buflen, 1, f);
		state = cmph_hash_state_load(buf, buflen);
		bmz->hashes[i] = state;
		free(buf);
	}

	DEBUGP("Reading m and n\n");
	fread(&(bmz->n), sizeof(cmph_uint32), 1, f);	
	fread(&(bmz->m), sizeof(cmph_uint32), 1, f);	

	bmz->g = (cmph_uint32 *)malloc(sizeof(cmph_uint32)*bmz->n);
	fread(bmz->g, bmz->n*sizeof(cmph_uint32), 1, f);
	#ifdef DEBUG
	fprintf(stderr, "G: ");
	for (i = 0; i < bmz->n; ++i) fprintf(stderr, "%u ", bmz->g[i]);
	fprintf(stderr, "\n");
	#endif
	return;
}
		

cmph_uint32 cmph_bmz_mphf_search(cmph_mphf_t *mphf, const char *key, cmph_uint32 keylen)
{
	cmph_bmz_mphf_data_t *bmz = mphf->data;
	cmph_uint32 h1 = cmph_hash(bmz->hashes[0], key, keylen) % bmz->n;
	cmph_uint32 h2 = cmph_hash(bmz->hashes[1], key, keylen) % bmz->n;
	DEBUGP("key: %s h1: %u h2: %u\n", key, h1, h2);
	if (h1 == h2 && ++h2 > bmz->n) h2 = 0;
	DEBUGP("key: %s g[h1]: %u g[h2]: %u edges: %u\n", key, bmz->g[h1], bmz->g[h2], bmz->m);
	return bmz->g[h1] + bmz->g[h2];
}
void cmph_bmz_mphf_destroy(cmph_mphf_t *mphf)
{
	cmph_bmz_mphf_data_t *data = (cmph_bmz_mphf_data_t *)mphf->data;
	free(data->g);	
	cmph_hash_state_destroy(data->hashes[0]);
	cmph_hash_state_destroy(data->hashes[1]);
	free(data->hashes);
	free(data);
	free(mphf);
}
