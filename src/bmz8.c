#include "graph.h"
#include "bmz8.h"
#include "cmph_structs.h"
#include "bmz8_structs.h"
#include "hash.h"
#include "vqueue.h"
#include "bitbool.h"
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

//#define DEBUG
#include "debug.h"

static int bmz8_gen_edges(cmph_config_t *mph);
static cmph_uint8 bmz8_traverse_critical_nodes(bmz8_config_data_t *bmz8, cmph_uint8 v, cmph_uint8 * biggest_g_value, cmph_uint8 * biggest_edge_value, cmph_uint8 * used_edges, cmph_uint8 * visited);
static cmph_uint8 bmz8_traverse_critical_nodes_heuristic(bmz8_config_data_t *bmz8, cmph_uint8 v, cmph_uint8 * biggest_g_value, cmph_uint8 * biggest_edge_value, cmph_uint8 * used_edges, cmph_uint8 * visited);
static void bmz8_traverse_non_critical_nodes(bmz8_config_data_t *bmz8, cmph_uint8 * used_edges, cmph_uint8 * visited);

bmz8_config_data_t *bmz8_config_new()
{
	bmz8_config_data_t *bmz8;
	bmz8 = (bmz8_config_data_t *)malloc(sizeof(bmz8_config_data_t));
	assert(bmz8);
	memset(bmz8, 0, sizeof(bmz8_config_data_t));
	bmz8->hashfuncs[0] = CMPH_HASH_JENKINS;
	bmz8->hashfuncs[1] = CMPH_HASH_JENKINS;
	bmz8->g = NULL;
	bmz8->graph = NULL;
	bmz8->hashes = NULL;
	return bmz8;
}

void bmz8_config_destroy(cmph_config_t *mph)
{
	bmz8_config_data_t *data = (bmz8_config_data_t *)mph->data;
	DEBUGP("Destroying algorithm dependent data\n");
	free(data);
}

void bmz8_config_set_hashfuncs(cmph_config_t *mph, CMPH_HASH *hashfuncs)
{
	bmz8_config_data_t *bmz8 = (bmz8_config_data_t *)mph->data;
	CMPH_HASH *hashptr = hashfuncs;
	cmph_uint8 i = 0;
	while(*hashptr != CMPH_HASH_COUNT)
	{
		if (i >= 2) break; //bmz8 only uses two hash functions
		bmz8->hashfuncs[i] = *hashptr;	
		++i, ++hashptr;
	}
}

cmph_t *bmz8_new(cmph_config_t *mph, float c)
{
	cmph_t *mphf = NULL;
	bmz8_data_t *bmz8f = NULL;
	cmph_uint8 i;
	cmph_uint8 iterations;
	cmph_uint8 iterations_map = 20;
	cmph_uint8 *used_edges = NULL;
	cmph_uint8 restart_mapping = 0;
	cmph_uint8 * visited = NULL;
	bmz8_config_data_t *bmz8 = (bmz8_config_data_t *)mph->data;
	
	if (mph->key_source->nkeys >= 256)
	{
		if (mph->verbosity) fprintf(stderr, "The number of keys in BMZ8 must be lower than 256.\n");
		return NULL;
	}
	if (c == 0) c = 1.15; // validating restrictions over parameter c.
	DEBUGP("c: %f\n", c);
	bmz8->m = mph->key_source->nkeys;	
	bmz8->n = ceil(c * mph->key_source->nkeys);	
	DEBUGP("m (edges): %u n (vertices): %u c: %f\n", bmz8->m, bmz8->n, c);
	bmz8->graph = graph_new(bmz8->n, bmz8->m);
	DEBUGP("Created graph\n");

	bmz8->hashes = (hash_state_t **)malloc(sizeof(hash_state_t *)*3);
	for(i = 0; i < 3; ++i) bmz8->hashes[i] = NULL;

	do
	{
	  // Mapping step
	  cmph_uint8 biggest_g_value = 0;
	  cmph_uint8 biggest_edge_value = 1;
	  iterations = 100;
	  if (mph->verbosity)
	  {
		fprintf(stderr, "Entering mapping step for mph creation of %u keys with graph sized %u\n", bmz8->m, bmz8->n);
	  }
	  while(1)
	  {
		int ok;
		DEBUGP("hash function 1\n");
		bmz8->hashes[0] = hash_state_new(bmz8->hashfuncs[0], bmz8->n);
		DEBUGP("hash function 2\n");
		bmz8->hashes[1] = hash_state_new(bmz8->hashfuncs[1], bmz8->n);
		DEBUGP("Generating edges\n");
		ok = bmz8_gen_edges(mph);
		if (!ok)
		{
			--iterations;
			hash_state_destroy(bmz8->hashes[0]);
			bmz8->hashes[0] = NULL;
			hash_state_destroy(bmz8->hashes[1]);
			bmz8->hashes[1] = NULL;
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
		graph_destroy(bmz8->graph);
		return NULL;
	  }

	  // Ordering step
	  if (mph->verbosity)
	  {
		fprintf(stderr, "Starting ordering step\n");
	  }

	  graph_obtain_critical_nodes(bmz8->graph);

	  // Searching step
	  if (mph->verbosity)
	  {
		fprintf(stderr, "Starting Searching step.\n");
		fprintf(stderr, "\tTraversing critical vertices.\n");
	  }
	  DEBUGP("Searching step\n");
	  visited = (cmph_uint8 *)malloc(bmz8->n/8 + 1);
	  memset(visited, 0, bmz8->n/8 + 1);
	  used_edges = (cmph_uint8 *)malloc(bmz8->m/8 + 1);
	  memset(used_edges, 0, bmz8->m/8 + 1);
	  free(bmz8->g);
	  bmz8->g = (cmph_uint8 *)calloc(bmz8->n, sizeof(cmph_uint8));
	  assert(bmz8->g);
	  for (i = 0; i < bmz8->n; ++i) // critical nodes
	  {
                if (graph_node_is_critical(bmz8->graph, i) && (!GETBIT(visited,i)))
		{
		  if(c > 1.14) restart_mapping = bmz8_traverse_critical_nodes(bmz8, i, &biggest_g_value, &biggest_edge_value, used_edges, visited);
		  else restart_mapping = bmz8_traverse_critical_nodes_heuristic(bmz8, i, &biggest_g_value, &biggest_edge_value, used_edges, visited);
		  if(restart_mapping) break;
		}
	  }
	  if(!restart_mapping)
	  {
	        if (mph->verbosity)
	        {
		  fprintf(stderr, "\tTraversing non critical vertices.\n");
		}
		bmz8_traverse_non_critical_nodes(bmz8, used_edges, visited); // non_critical_nodes
	  }
	  else 
	  {
 	        iterations_map--;
		if (mph->verbosity) fprintf(stderr, "Restarting mapping step. %u iterations remaining.\n", iterations_map);
	  }    

	  free(used_edges);
	  free(visited);

	}while(restart_mapping && iterations_map > 0);
	graph_destroy(bmz8->graph);	
	bmz8->graph = NULL;
	if (iterations_map == 0) 
	{
		return NULL;
	}
	mphf = (cmph_t *)malloc(sizeof(cmph_t));
	mphf->algo = mph->algo;
	bmz8f = (bmz8_data_t *)malloc(sizeof(bmz8_data_t));
	bmz8f->g = bmz8->g;
	bmz8->g = NULL; //transfer memory ownership
	bmz8f->hashes = bmz8->hashes;
	bmz8->hashes = NULL; //transfer memory ownership
	bmz8f->n = bmz8->n;
	bmz8f->m = bmz8->m;
	mphf->data = bmz8f;
	mphf->size = bmz8->m;
	DEBUGP("Successfully generated minimal perfect hash\n");
	if (mph->verbosity)
	{
		fprintf(stderr, "Successfully generated minimal perfect hash function\n");
	}
	return mphf;
}

static cmph_uint8 bmz8_traverse_critical_nodes(bmz8_config_data_t *bmz8, cmph_uint8 v, cmph_uint8 * biggest_g_value, cmph_uint8 * biggest_edge_value, cmph_uint8 * used_edges, cmph_uint8 * visited)
{
        cmph_uint8 next_g;
	cmph_uint32 u;   /* Auxiliary vertex */
	cmph_uint32 lav; /* lookahead vertex */
	cmph_uint8 collision;
	vqueue_t * q = vqueue_new((cmph_uint32)(graph_ncritical_nodes(bmz8->graph)));
	graph_iterator_t it, it1;

	DEBUGP("Labelling critical vertices\n");
	bmz8->g[v] = (cmph_uint8)ceil ((double)(*biggest_edge_value)/2) - 1;
	SETBIT(visited, v);
	next_g    = (cmph_uint8)floor((double)(*biggest_edge_value/2)); /* next_g is incremented in the do..while statement*/
	vqueue_insert(q, v);
	while(!vqueue_is_empty(q))
	{
		v = vqueue_remove(q);
		it = graph_neighbors_it(bmz8->graph, v);		
		while ((u = graph_next_neighbor(bmz8->graph, &it)) != GRAPH_NO_NEIGHBOR)
		{		        
               	        if (graph_node_is_critical(bmz8->graph, u) && (!GETBIT(visited,u)))
			{
			        collision = 1;
			        while(collision) // lookahead to resolve collisions
				{
 				        next_g = *biggest_g_value + 1; 
					it1 = graph_neighbors_it(bmz8->graph, u);
					collision = 0;
					while((lav = graph_next_neighbor(bmz8->graph, &it1)) != GRAPH_NO_NEIGHBOR)
					{
  					        if (graph_node_is_critical(bmz8->graph, lav) && GETBIT(visited,lav))
						{
   						        if(next_g + bmz8->g[lav] >= bmz8->m)
							{
							        vqueue_destroy(q);
								return 1; // restart mapping step.
							}
							if (GETBIT(used_edges, next_g + bmz8->g[lav])) 
							{
							        collision = 1;
								break;
							}
						}
					}
 					if (next_g > *biggest_g_value) *biggest_g_value = next_g;
				}		
				// Marking used edges...
				it1 = graph_neighbors_it(bmz8->graph, u);
				while((lav = graph_next_neighbor(bmz8->graph, &it1)) != GRAPH_NO_NEIGHBOR)
				{
                 		        if (graph_node_is_critical(bmz8->graph, lav) && GETBIT(visited, lav))
					{
                   			        SETBIT(used_edges,next_g + bmz8->g[lav]);
						if(next_g + bmz8->g[lav] > *biggest_edge_value) *biggest_edge_value = next_g + bmz8->g[lav];
					}
				}
				bmz8->g[u] = next_g; // Labelling vertex u.
				SETBIT(visited,u);
			        vqueue_insert(q, u);
			}			
	        }
		 
	}
	vqueue_destroy(q);
	return 0;
}

static cmph_uint8 bmz8_traverse_critical_nodes_heuristic(bmz8_config_data_t *bmz8, cmph_uint8 v, cmph_uint8 * biggest_g_value, cmph_uint8 * biggest_edge_value, cmph_uint8 * used_edges, cmph_uint8 * visited)
{
        cmph_uint8 next_g;
	cmph_uint32 u;   
	cmph_uint32 lav; 
	cmph_uint8 collision;
	cmph_uint8 * unused_g_values = NULL;
	cmph_uint8 unused_g_values_capacity = 0;
	cmph_uint8 nunused_g_values = 0;
	vqueue_t * q = vqueue_new((cmph_uint32)(graph_ncritical_nodes(bmz8->graph)));
	graph_iterator_t it, it1;

	DEBUGP("Labelling critical vertices\n");
	bmz8->g[v] = (cmph_uint8)ceil ((double)(*biggest_edge_value)/2) - 1;
	SETBIT(visited, v);
	next_g    = (cmph_uint8)floor((double)(*biggest_edge_value/2)); 
	vqueue_insert(q, v);
	while(!vqueue_is_empty(q))
	{
		v = vqueue_remove(q);
		it = graph_neighbors_it(bmz8->graph, v);		
		while ((u = graph_next_neighbor(bmz8->graph, &it)) != GRAPH_NO_NEIGHBOR)
		{		        
               	        if (graph_node_is_critical(bmz8->graph, u) && (!GETBIT(visited,u)))
			{
			        cmph_uint8 next_g_index = 0;
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
						next_g_index = 255;//UINT_MAX;
					}
					it1 = graph_neighbors_it(bmz8->graph, u);
					collision = 0;
					while((lav = graph_next_neighbor(bmz8->graph, &it1)) != GRAPH_NO_NEIGHBOR)
					{
  					        if (graph_node_is_critical(bmz8->graph, lav) && GETBIT(visited,lav))
						{
   						        if(next_g + bmz8->g[lav] >= bmz8->m)
							{
							        vqueue_destroy(q);
								free(unused_g_values);
								return 1; // restart mapping step.
							}
							if (GETBIT(used_edges, next_g + bmz8->g[lav])) 
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
							unused_g_values = (cmph_uint8*)realloc(unused_g_values, (unused_g_values_capacity + BUFSIZ)*sizeof(cmph_uint8));
						        unused_g_values_capacity += BUFSIZ;  							
						} 
						unused_g_values[nunused_g_values++] = next_g;							

					}
 					if (next_g > *biggest_g_value) *biggest_g_value = next_g;
				}
				
				next_g_index--;
				if (next_g_index < nunused_g_values) unused_g_values[next_g_index] = unused_g_values[--nunused_g_values];

				// Marking used edges...
				it1 = graph_neighbors_it(bmz8->graph, u);
				while((lav = graph_next_neighbor(bmz8->graph, &it1)) != GRAPH_NO_NEIGHBOR)
				{
                 		        if (graph_node_is_critical(bmz8->graph, lav) && GETBIT(visited, lav))
					{
                   			        SETBIT(used_edges,next_g + bmz8->g[lav]);
						if(next_g + bmz8->g[lav] > *biggest_edge_value) *biggest_edge_value = next_g + bmz8->g[lav];
					}
				}
				
				bmz8->g[u] = next_g; // Labelling vertex u.
				SETBIT(visited, u);
			      	vqueue_insert(q, u);
				
			}			
	        }
		 
	}
	vqueue_destroy(q);
	free(unused_g_values);
	return 0;  
}

static cmph_uint8 next_unused_edge(bmz8_config_data_t *bmz8, cmph_uint8 * used_edges, cmph_uint8 unused_edge_index)
{
       while(1)
       {
		assert(unused_edge_index < bmz8->m);
		if(GETBIT(used_edges, unused_edge_index)) unused_edge_index ++;
		else break;
       }
       return unused_edge_index;
}

static void bmz8_traverse(bmz8_config_data_t *bmz8, cmph_uint8 * used_edges, cmph_uint8 v, cmph_uint8 * unused_edge_index, cmph_uint8 * visited)
{
	graph_iterator_t it = graph_neighbors_it(bmz8->graph, v);
	cmph_uint32 neighbor = 0;
	while((neighbor = graph_next_neighbor(bmz8->graph, &it)) != GRAPH_NO_NEIGHBOR)
	{
     	        if(GETBIT(visited,neighbor)) continue;
		//DEBUGP("Visiting neighbor %u\n", neighbor);
		*unused_edge_index = next_unused_edge(bmz8, used_edges, *unused_edge_index);
		bmz8->g[neighbor] = *unused_edge_index - bmz8->g[v];
		//if (bmz8->g[neighbor] >= bmz8->m) bmz8->g[neighbor] += bmz8->m;
		SETBIT(visited, neighbor);
		(*unused_edge_index)++;
		bmz8_traverse(bmz8, used_edges, neighbor, unused_edge_index, visited);
		
	}  
}

static void bmz8_traverse_non_critical_nodes(bmz8_config_data_t *bmz8, cmph_uint8 * used_edges, cmph_uint8 * visited)
{

	cmph_uint8 i, v1, v2, unused_edge_index = 0;
	DEBUGP("Labelling non critical vertices\n");
	for(i = 0; i < bmz8->m; i++)
	{
	        v1 = graph_vertex_id(bmz8->graph, i, 0);
		v2 = graph_vertex_id(bmz8->graph, i, 1);
		if((GETBIT(visited,v1) && GETBIT(visited,v2)) || (!GETBIT(visited,v1) && !GETBIT(visited,v2))) continue;				  	
		if(GETBIT(visited,v1)) bmz8_traverse(bmz8, used_edges, v1, &unused_edge_index, visited);
	        else bmz8_traverse(bmz8, used_edges, v2, &unused_edge_index, visited);

	}

	for(i = 0; i < bmz8->n; i++)
	{
		if(!GETBIT(visited,i))
		{ 	                        
		        bmz8->g[i] = 0;
			SETBIT(visited, i);
			bmz8_traverse(bmz8, used_edges, i, &unused_edge_index, visited);
		}
	}

}
		
static int bmz8_gen_edges(cmph_config_t *mph)
{
	cmph_uint8 e;
	bmz8_config_data_t *bmz8 = (bmz8_config_data_t *)mph->data;
	cmph_uint8 multiple_edges = 0;
	DEBUGP("Generating edges for %u vertices\n", bmz8->n);
	graph_clear_edges(bmz8->graph);	
	mph->key_source->rewind(mph->key_source->data);
	for (e = 0; e < mph->key_source->nkeys; ++e)
	{
		cmph_uint8 h1, h2;
		cmph_uint32 keylen;
		char *key = NULL;
		mph->key_source->read(mph->key_source->data, &key, &keylen);
		
//		if (key == NULL)fprintf(stderr, "key = %s -- read BMZ\n", key);
		h1 = hash(bmz8->hashes[0], key, keylen) % bmz8->n;
		h2 = hash(bmz8->hashes[1], key, keylen) % bmz8->n;
		if (h1 == h2) if (++h2 >= bmz8->n) h2 = 0;
		if (h1 == h2) 
		{
			if (mph->verbosity) fprintf(stderr, "Self loop for key %u\n", e);
			mph->key_source->dispose(mph->key_source->data, key, keylen);
			return 0;
		}
		//DEBUGP("Adding edge: %u -> %u for key %s\n", h1, h2, key);
		mph->key_source->dispose(mph->key_source->data, key, keylen);
//		fprintf(stderr, "key = %s -- dispose BMZ\n", key);
		multiple_edges = graph_contains_edge(bmz8->graph, h1, h2);
		if (mph->verbosity && multiple_edges) fprintf(stderr, "A non simple graph was generated\n");
		if (multiple_edges) return 0; // checking multiple edge restriction.
		graph_add_edge(bmz8->graph, h1, h2);
	}
	return !multiple_edges;
}

int bmz8_dump(cmph_t *mphf, FILE *fd)
{
	char *buf = NULL;
	cmph_uint32 buflen;
	cmph_uint8 two = 2; //number of hash functions
	bmz8_data_t *data = (bmz8_data_t *)mphf->data;
	__cmph_dump(mphf, fd);

	fwrite(&two, sizeof(cmph_uint8), 1, fd);

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

	fwrite(&(data->n), sizeof(cmph_uint8), 1, fd);
	fwrite(&(data->m), sizeof(cmph_uint8), 1, fd);
	
	fwrite(data->g, sizeof(cmph_uint8)*(data->n), 1, fd);
	#ifdef DEBUG
	fprintf(stderr, "G: ");
	for (i = 0; i < data->n; ++i) fprintf(stderr, "%u ", data->g[i]);
	fprintf(stderr, "\n");
	#endif
	return 1;
}

void bmz8_load(FILE *f, cmph_t *mphf)
{
	cmph_uint8 nhashes;
	char *buf = NULL;
	cmph_uint32 buflen;
	cmph_uint8 i;
	bmz8_data_t *bmz8 = (bmz8_data_t *)malloc(sizeof(bmz8_data_t));

	DEBUGP("Loading bmz8 mphf\n");
	mphf->data = bmz8;
	fread(&nhashes, sizeof(cmph_uint8), 1, f);
	bmz8->hashes = (hash_state_t **)malloc(sizeof(hash_state_t *)*(nhashes + 1));
	bmz8->hashes[nhashes] = NULL;
	DEBUGP("Reading %u hashes\n", nhashes);
	for (i = 0; i < nhashes; ++i)
	{
		hash_state_t *state = NULL;
		fread(&buflen, sizeof(cmph_uint32), 1, f);
		DEBUGP("Hash state has %u bytes\n", buflen);
		buf = (char *)malloc(buflen);
		fread(buf, buflen, 1, f);
		state = hash_state_load(buf, buflen);
		bmz8->hashes[i] = state;
		free(buf);
	}

	DEBUGP("Reading m and n\n");
	fread(&(bmz8->n), sizeof(cmph_uint8), 1, f);	
	fread(&(bmz8->m), sizeof(cmph_uint8), 1, f);	

	bmz8->g = (cmph_uint8 *)malloc(sizeof(cmph_uint8)*bmz8->n);
	fread(bmz8->g, bmz8->n*sizeof(cmph_uint8), 1, f);
	#ifdef DEBUG
	fprintf(stderr, "G: ");
	for (i = 0; i < bmz8->n; ++i) fprintf(stderr, "%u ", bmz8->g[i]);
	fprintf(stderr, "\n");
	#endif
	return;
}
		

cmph_uint8 bmz8_search(cmph_t *mphf, const char *key, cmph_uint32 keylen)
{
	bmz8_data_t *bmz8 = mphf->data;
	cmph_uint8 h1 = hash(bmz8->hashes[0], key, keylen) % bmz8->n;
	cmph_uint8 h2 = hash(bmz8->hashes[1], key, keylen) % bmz8->n;
	DEBUGP("key: %s h1: %u h2: %u\n", key, h1, h2);
	if (h1 == h2 && ++h2 > bmz8->n) h2 = 0;
	DEBUGP("key: %s g[h1]: %u g[h2]: %u edges: %u\n", key, bmz8->g[h1], bmz8->g[h2], bmz8->m);
	return (bmz8->g[h1] + bmz8->g[h2]);
}
void bmz8_destroy(cmph_t *mphf)
{
	bmz8_data_t *data = (bmz8_data_t *)mphf->data;
	free(data->g);
	hash_state_destroy(data->hashes[0]);
	hash_state_destroy(data->hashes[1]);
	free(data->hashes);
	free(data);
	free(mphf);
}
