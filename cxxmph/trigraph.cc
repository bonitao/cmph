#include <cassert>
#include <limits>

#include "trigraph.h"

using std::vector;

namespace {
static const cmph_uint8 kInvalidEdge = std::numeric_limits<cmph_uint8>::max();
} 

namespace cxxmph {

TriGraph::TriGraph(cmph_uint32 nedges, cmph_uint32 nvertices)
      : nedges_(0),
        edges_(nedges),
        first_edge_(nvertices, kInvalidEdge),
        vertex_degree_(nvertices, 0) { }

void TriGraph::ExtractEdgesAndClear(vector<Edge>* edges) {
  vector<Edge>().swap(next_edge_);
  vector<cmph_uint32>().swap(first_edge_);
  vector<cmph_uint8>().swap(vertex_degree_);
  nedges_ = 0;
  edges->swap(edges_);
}
void TriGraph::AddEdge(const Edge& edge) { 
  edges_[nedges_] = edge;
  next_edge_[nedges_] = Edge(
      first_edge_[edge[0]], first_edge_[edge[1]], first_edge_[edge[2]]);
   first_edge_[edge[0]] = first_edge_[edge[1]] = first_edge_[edge[2]] = nedges_;
   ++vertex_degree_[edge[0]];
   ++vertex_degree_[edge[1]];
   ++vertex_degree_[edge[2]];
   ++nedges_;
}

void TriGraph::RemoveEdge(cmph_uint32 current_edge) {
  cmph_uint32 vertex, edge1, edge2;
  for (int i = 0; i < 3; ++i) {
    cmph_uint32 vertex = edges_[current_edge][i];
    cmph_uint32 edge1 = first_edge_[vertex];
    cmph_uint32 edge2 = kInvalidEdge;
    cmph_uint32 j = 0;
    while (edge1 != current_edge && edge1 != kInvalidEdge) {
      edge2 = edge1;
      if (edges_[edge1][0] == vertex) j = 0;
      else if (edges_[edge1][1] == vertex) j = 1;
      else j = 2;
      edge1 = next_edge_[edge1][j];
    }
    assert(edge1 != kInvalidEdge);
    if (edge2 != kInvalidEdge) next_edge_[edge2][j] = next_edge_[edge1][i];
    else first_edge_[vertex] = next_edge_[edge1][i];
    --vertex_degree_[vertex];
  }
}
     
}  // namespace cxxmph
