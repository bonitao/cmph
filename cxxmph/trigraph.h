#ifndef __CXXMPH_TRIGRAPH_H__
#define __CXXMPH_TRIGRAPH_H__
// Build a trigraph using a memory efficient representation.
//
// Prior knowledge of the number of edges and vertices for the graph is
// required. For each vertex, we store how many edges touch it (degree) and the
// index of the first edge in the vector of triples representing the edges.


#include <vector>

#include "../src/cmph_types.h"

namespace cxxmph {

class TriGraph {
  struct Edge {
    Edge() { }
    Edge(cmph_uint32 v0, cmph_uint32 v1, cmph_uint32 v2);
    cmph_uint32& operator[](cmph_uint8 v) { return vertices[v]; }
    const cmph_uint32& operator[](cmph_uint8 v) const { return vertices[v]; }
    cmph_uint32 vertices[3];
  };
  TriGraph(cmph_uint32 nedges, cmph_uint32 nvertices);
  void AddEdge(const Edge& edge);
  void RemoveEdge(cmph_uint32 edge_id);
  void ExtractEdgesAndClear(std::vector<Edge>* edges);

  const std::vector<Edge>& edges() const { return edges_; }
  const std::vector<cmph_uint8>& vertex_degree() const { return vertex_degree_; }
  const std::vector<cmph_uint32>& first_edge() const { return first_edge_; }

 private:
  cmph_uint32 nedges_;  // total number of edges
  std::vector<Edge> edges_;
  std::vector<Edge> next_edge_;  // for implementing removal
  std::vector<cmph_uint32> first_edge_;  // the first edge for this vertex
  std::vector<cmph_uint8> vertex_degree_;  // number of edges for this vertex
};

}  // namespace cxxmph

#endif  // __CXXMPH_TRIGRAPH_H__
