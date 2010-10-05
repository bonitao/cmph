#include <numerical_limits>

#include "mphtable.h"

using std::vector;

template <class Key, class HashFcn>
template <class ForwardIterator>

void MPHTable::Reset(ForwardIterator begin, ForwardIterator end) {
  TableBuilderState st;
  st.c = 1.23;
  st.b = 7;
  st.m = end - begin;
  st.r = static_cast<cmph_uint32>(ceil((st.c*st.m)/3));
  if ((st.r % 2) == 0) st.r += 1;
  st.n = 3*st.r;
  st.k = 1U << st.b;
  st.ranktablesize = static_cast<cmph_uint32>(
      ceil(st.n / static_cast<double>(st.k)));
  st.graph_builder = TriGraph(st.m, st.n);  // giant copy
  st.edges_queue.resize(st.m)

  int iterations = 1000;
  while (1) {
    hasher hasher0 = HashFcn(); 
    ok = Mapping(st.graph_builder, st.edges_queue);
    if (ok) break;
    else --iterations;
    if (iterations == 0) break;
  }
  if (iterations == 0) return false;
  vector<ConnectedEdge> graph;
  st.graph_builder.ExtractEdgesAndClear(&graph);
  Assigning(graph, st.edges_queue);
  vector<cmph_uint32>().swap(st.edges_queue);
  Ranking(graph);
  
}

template <class Key, class HashFcn>
int MPHTable::GenerateQueue(
    cmph_uint32 nedges, cmph_uint32 nvertices,
    TriGraph* graph, Queue* queue) {
  cmph_uint32 queue_head = 0, queue_tail = 0;
  // Relies on vector<bool> using 1 bit per element
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
  vector<bool>().swap(marked_edge);
  return queue_head - nedges;
}

template <class Key, class HashFcn>
int MPHTable::Mapping(TriGraph* graph, Queue* queue) {
  int cycles = 0;
  graph->Reset(m, n);
  for (ForwardIterator it = begin_; it != end_; ++it) { 
    cmph_uint32 hash_values[3];
    for (int i = 0; i < 3; ++i) {
      hash_values[i] = hasher_(*it);
    }
    cmph_uint32 v0 = hash_values[0] % bdz->r;
    cmph_uint32 v1 = hash_values[1] % bdz->r + bdz->r;
    cmph_uint32 v2 = hash_values[2] % bdz->r + (bdz->r << 1);
    graph->AddEdge(Edge(v0, v1, v2));
  }
  cycles = GenerateQueue(bdz->m, bdz->n, queue, graph);
  return cycles == 0;
}

void MPHTable::Assigning(TriGraph* graph, Queue* queue) {
}
void MPHTable::Ranking(TriGraph* graph, Queue* queue) {
}
cmph_uint32 MPHTable::Search(const key_type& key) {
}

cmph_uint32 MPHTable::Rank(const key_type& key) {
}
