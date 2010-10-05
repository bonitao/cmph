#include <limits>

#include "trigraph.h"

using std::vector;

namespace {
static const cmph_uint8 kInvalidEdge = std::numeric_limits<cmph_uint8>::max();
} 

TriGraph::TriGraph(cmph_uint32 nedges, cmph_uint32 nvertices)
      : nedges_(0),
        edges_(nedges),
        first_edge_(nvertices, kInvalidEdge),
        vertex_degree_(nvertices, 0) { }

void TriGraph::ExtractEdgesAndClear(vector<ConnectedEdge>* edges) {
  vector<cmph_uint32>().swap(first_edge_);
  vector<cmph_uint8>().swap(vertex_degree_);
  nedges_ = 0;
  edges->swap(edges_);
}
void TriGraph::AddEdge(const Edge& edge) { }
void TriGraph::RemoveEdge(cmph_uint32 current_edge) { }
