#ifndef _CMPH_GRAPH_H__
#define _CMPH_GRAPH_H__

#include <limits.h>
#include "cmph_types.h"

#define GRAPH_NO_NEIGHBOR UINT_MAX

typedef struct __graph_t graph_t;
typedef struct __graph_iterator_t graph_iterator_t;
struct __graph_iterator_t
{
	uint32 vertex;
	uint32 edge;
};



graph_t *graph_new(uint32 nnodes, uint32 nedges);
void graph_destroy(graph_t *graph);

void graph_add_edge(graph_t *g, uint32 v1, uint32 v2);
void graph_del_edge(graph_t *g, uint32 v1, uint32 v2);
void graph_clear_edges(graph_t *g);
uint32 graph_edge_id(graph_t *g, uint32 v1, uint32 v2);
uint8 graph_contains_edge(graph_t *g, uint32 v1, uint32 v2);

graph_iterator_t graph_neighbors_it(graph_t *g, uint32 v);
uint32 graph_next_neighbor(graph_t *g, graph_iterator_t* it);

void graph_obtain_critical_nodes(graph_t *g);            /* included -- Fabiano*/
uint8 graph_node_is_critical(graph_t * g, uint32 v);     /* included -- Fabiano */
uint32 graph_ncritical_nodes(graph_t *g);                /* included -- Fabiano*/
uint32 graph_vertex_id(graph_t *g, uint32 e, uint32 id); /* included -- Fabiano*/

int graph_is_cyclic(graph_t *g);

void graph_print(graph_t *);

#endif
