#ifndef _CMPH_GRAPH_H__
#define _CMPH_GRAPH_H__

#include <limits.h>
#include "cmph_types.h"

#define CMPH_GRAPH_NO_NEIGHBOR UINT_MAX

typedef struct cmph__graph_t cmph_graph_t;
typedef struct cmph__graph_iterator_t cmph_graph_iterator_t;
struct cmph__graph_iterator_t
{
	cmph_uint32 vertex;
	cmph_uint32 edge;
};



cmph_graph_t *cmph_graph_new(cmph_uint32 nnodes, cmph_uint32 nedges);
void cmph_graph_destroy(cmph_graph_t *graph);

void cmph_graph_add_edge(cmph_graph_t *g, cmph_uint32 v1, cmph_uint32 v2);
void cmph_graph_del_edge(cmph_graph_t *g, cmph_uint32 v1, cmph_uint32 v2);
void cmph_graph_clear_edges(cmph_graph_t *g);
cmph_uint32 cmph_graph_edge_id(cmph_graph_t *g, cmph_uint32 v1, cmph_uint32 v2);
cmph_uint8 cmph_graph_contains_edge(cmph_graph_t *g, cmph_uint32 v1, cmph_uint32 v2);

cmph_graph_iterator_t cmph_graph_neighbors_it(cmph_graph_t *g, cmph_uint32 v);
cmph_uint32 cmph_graph_next_neighbor(cmph_graph_t *g, cmph_graph_iterator_t* it);

void cmph_graph_obtain_critical_nodes(cmph_graph_t *g);            /* included -- Fabiano*/
cmph_uint8 cmph_graph_node_is_critical(cmph_graph_t * g, cmph_uint32 v);     /* included -- Fabiano */
cmph_uint32 cmph_graph_ncritical_nodes(cmph_graph_t *g);                /* included -- Fabiano*/
cmph_uint32 cmph_graph_vertex_id(cmph_graph_t *g, cmph_uint32 e, cmph_uint32 id); /* included -- Fabiano*/

int cmph_graph_is_cyclic(cmph_graph_t *g);

void cmph_graph_print(cmph_graph_t *);

#endif
