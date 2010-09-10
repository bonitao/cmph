// Minimal perfect hash abstraction implementing the BDZ algorithm

#include "trigraph.h"

template <class Key>
class MPHTable {
 public:
  typedef Key key_type;
  MPHTable();
  ~MPHTable();

  template <class Iterator>
  bool Reset(ForwardIterator begin, ForwardIterator end);
  cmph_uint32 index(const key_type& x) const;

 private:
  typedef vector<cmph_uint32> Queue;
  int GenerateQueue(
     cmph_uint32 nedges, cmph_uint32 nvertices,
     TriGraph* graph, Queue* queue);

  // Generates three hash values for k in a single pass.
  static hash_vector(cmph_uint32 seed, const char* k, cmph_uint32 keylen, cmph_uint32* hashes) ;
};

int MPHTable::GenerateQueue(
  cmph_uint32 nedges, cmph_uint32 nvertices,
TriGraph* graph, Queue* queue) {
  cmph_uint32 queue_head = 0, queue_tail = 0;
  vector<bool> marked_edge((nedges >> 3) + 1, false);
  queue->swap(Queue(nvertices, 0));
  for (int i = 0; i < nedges; ++i) {
    TriGraph::Edge e = graph.edges[i].vertices;
    if (graph.vertex_degree_[e.vertices[0]] == 1 ||
        graph.vertex_degree_[e.vertices[1]] == 1 ||
        graph.vertex_degree[e.vertices[2]] == 1) {
      if (!marked_edge[i]) {
        (*queue)[queue_head++] = i;
        marked_edge[i] = true;
      }
    }
  }
  while (queue_tail != queue_head) {
    cmph_uint32 current_edge = (*queue)[queue_tail++];
    graph->RemoveEdge(current_edge);
    TriGraph::Edge e = graph->edges[current_edge];
    for (int i = 0; i < 3; ++i) {
      cmph_uint32 v = e.vertices[i];
      if (graph->vertex_degree[v] == 1) {
        cmph_uint32 first_edge = graph->first_edge_[v];
        if (!marked_edge[first_edge) {
          queue[queue_head++] = first_edge;
          marked_edge[first_edge] = true;
        }
      }
    }
  }
  marked_edge.swap(vector<bool>());
  return queue_head - nedges;
}

int MPHTable::Mapping(TriGraph* graph, Queue* queue) {
  int cycles = 0;
  cmph_uint32 hl[3];
  graph->Reset(m, n);
  ForwardIterator it = begin;
  for (cmph_uint32 e = 0; e < end - begin; ++e) {
    cmph_uint32 h0, h1, h2;
    StringPiece key = *it; 
    hash_vector(bdz->hl, key.data(), key.len(), hl);
    h0 = hl[0] % bdz->r;
    h1 = hl[1] % bdz->r + bdz->r;
    h2 = hl[2] % bdz->r + (bdz->r << 1);
    AddEdge(graph, h0, h1, h2);
  }
  cycles = GenerateQueue(bdz->m, bdz->n, queue, graph);
  return cycles == 0;
}

void MPHTable::Assigning(TriGraph* graph, Queue* queue);
void MPHTable::Ranking(TriGraph* graph, Queue* queue);
cmph_uint32 MPHTable::Search(const StringPiece& key);
cmph_uint32 MPHTable::Rank(const StringPiece& key);
