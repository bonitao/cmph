#include "trigraph.h"

namespace {
static const cmph_uint8 kInvalidEdge = std::limits<cmph_uint8>::max;
} 

TriGraph::TriGraph(cmph_uint32 nedges, cmph_uint32 nvertices)
      : nedges_(0),
        edges_(nedges, 0),
        first_edge_(nvertices, kInvalidEdge),
        vertex_degree_(nvertices, 0) { }

void Trigraph::ExtractEdgesAndClear(vector<ConnectedEdge>* edges) {
  first_edge_.swap(vector<cmph_uint32>());
  vertex_degree_.swap(vector<cmph_uint8>());
  nedges_ = 0;
  edges->swap(edges_);
}
void TriGraph::AddEdge(const Edge& edge) { }
void TriGraph::RemoveEdge(cmph_uint32 current_edge) { }
