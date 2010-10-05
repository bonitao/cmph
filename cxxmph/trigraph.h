#include <vector>

#include "../src/cmph_types.h"

class TriGraph {
  struct Edge {
    Edge() { }
    Edge(cmph_uint32 v0, cmph_uint32 v1, cmph_uint32 v2);
    cmph_uint32 vertices[3];
  };
  struct ConnectedEdge {
    Edge current;
    Edge next;
  };

  TriGraph(cmph_uint32 nedges, cmph_uint32 nvertices);
  void AddEdge(const Edge& edge);
  void RemoveEdge(cmph_uint32 current_edge);
  void ExtractEdgesAndClear(std::vector<ConnectedEdge>* edges);

 private:
  cmph_uint32 nedges_;
  std::vector<ConnectedEdge> edges_;
  std::vector<cmph_uint32> first_edge_;
  std::vector<cmph_uint8> vertex_degree_;
};
