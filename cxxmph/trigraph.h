class TriGraph {
  struct Edge {
    Edge(cmph_uint32 v0, cmph_uint32 v1, cmph_uint32 v2);
    cmph_uint32 vertices[3];
  };
  struct ConnectedEdge {
    Edge current;
    Edge next;
  };

  TriGraph(cmph_uint32 nedges, cmph_uint32 nvertices);
  void AddEdge(cmph_uint32 v0, cmph_uint32 v1, cmph_uint32 v2);
  void RemoveEdge(cmph_uint32 current_edge);
  void ExtractEdgesAndClear(vector<ConnectedEdge>* edges);

 private:
  cmph_uint32 nedges_;
  vector<ConnectedEdge> edges_;
  vector<cmph_uint32> first_edge_;
  vector<cmph_uint8> vertex_degree_;
};
