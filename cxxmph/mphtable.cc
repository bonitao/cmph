#include <limits>

#include "mphtable.h"

using std::vector;

namespace cxxmph {

template <class Key, class HashFcn>
template <class ForwardIterator>
bool MPHTable<Key, HashFcn>::Reset(ForwardIterator begin, ForwardIterator end) {
  TableBuilderState<ForwardIterator> st;
  m_ = end - begin;
  r_ = static_cast<cmph_uint32>(ceil((c_*m_)/3));
  if (r_ % 2) == 0) r_ += 1;
  n_ = 3*r_;
  k_ = 1U << b_;

  int iterations = 1000;
  while (1) {
    for (int i = 0; i < 3; ++i) hash_function_[i] = hasher();
    vector<Edge> edges;
    vector<cmph_uint32> queue;
    if (Mapping(begin, end, &edges, &queue)) break;
    else --iterations;
    if (iterations == 0) break;
  }
  if (iterations == 0) return false;
  vector<Edge>& edges;
  graph->ExtractEdgesAndClear(&edges);
  Assigning(queue, edges);
  vector<cmph_uint32>().swap(edges);
  Ranking();
  
}

template <class Key, class HashFcn>
bool MPHTable<Key, HashFcn>::GenerateQueue(
    TriGraph* graph, vector<cmph_uint32>* queue_output) {
  cmph_uint32 queue_head = 0, queue_tail = 0;
  cmph_uint32 nedges = n_;
  cmph_uint32 nvertices = m_;
  // Relies on vector<bool> using 1 bit per element
  vector<bool> marked_edge((nedges >> 3) + 1, false);
  Queue queue(nvertices, 0);
  for (int i = 0; i < nedges; ++i) {
    const TriGraph::Edge& e = graph->edges()[i];
    if (graph->vertex_degree()[e[0]] == 1 ||
        graph->vertex_degree()[e[1]] == 1 ||
        graph->vertex_degree()[e[2]] == 1) {
      if (!marked_edge[i]) {
        queue[queue_head++] = i;
        marked_edge[i] = true;
      }
    }
  }
  while (queue_tail != queue_head) {
    cmph_uint32 current_edge = queue[queue_tail++];
    graph->RemoveEdge(current_edge);
    const TriGraph::Edge& e = graph->edges()[current_edge];
    for (int i = 0; i < 3; ++i) {
      cmph_uint32 v = e[i];
      if (graph->vertex_degree()[v] == 1) {
        cmph_uint32 first_edge = graph->first_edge()[v];
        if (!marked_edge[first_edge]) {
          queue[queue_head++] = first_edge;
          marked_edge[first_edge] = true;
        }
      }
    }
  }
  int cycles = queue_head - nedges;
  if (cycles == 0) queue.swap(*queue_output);
  return cycles == 0;
}

template <class Key, class HashFcn>
template <class ForwardIterator>
bool MPHTable<Key, HashFcn>::Mapping(
    ForwardIterator begin, ForwardIterator end,
    vector<Edge>* edges, vector<cmph_uint32> queue) {
  int cycles = 0;
  TriGraph graph(m, n);
  for (ForwardIterator it = begin; it != end; ++it) { 
    cmph_uint32 h[3];
    for (int i = 0; i < 3; ++i) h[i] = hash_function_[i](*it);
    cmph_uint32 v0 = h[0] % r_;
    cmph_uint32 v1 = h[1] % r_ + r_;
    cmph_uint32 v2 = h[2] % r_ + (r_ << 1);
    graph.AddEdge(Edge(v0, v1, v2));
  }
  if (GenerateQueue(&graph, queue)) {
     graph.ExtractEdgesAndClear(edges);
     return true;
  }
  return false;
}

template <class Key, class HashFcn>
void MPHTable<Key, HashFcn>::Assigning(
    const vector<Edge>& edges, const vector<cmph_uint32>& queue) {
  cmph_uint32 nedges = n_;
  cmph_uint32 current_edge = 0;
  vector<bool> marked_vertices(nedges + 1);
  // TODO(davi) use half nibbles instead
  // vector<cmph_uint8> g(static_cast<cmph_uint32>(ceil(nedges / 4.0)),
  //                     std::numerical_limits<cmph_uint8>::max());
  static const cmph_uint8 kUnassigned = 3;
  vector<cmph_uint8>(nedges, kUnassigned).swap(g_);
  for (int i = nedges - 1; i + 1 >= 1; --i) {
    current_edge = queue[i];
    const TriGraph::Edge& e = edges[current_edge];
    if (!marked_vertices[e[0]]) {
      if (!marked_vertices[e[1]]) {
        g_[e[1]] = kUnassigned;
        marked_vertices[e[1]] = true;
      }
      if (!marked_vertices[e[2]]) {
        g_[e[2]] = kUnassigned;
        marked_vertices[e[2]] = true;
      }
      g_[e[0]] = (6 - g_[e[1]] + g_[e2]) % 3;
      marked_vertices[e[0]] = true;
    } else if (!marked_vertices[e[1]])) {
      if (!marked_vertices[e[2]])) {
        g_[e[2]] = kUnassigned;
        marked_vertices[e[2]] = true;
      }
      g_[e[1]] = 7 - (g_[e[0]] + g_[e[2]]) % 3;
      marked_vertices[e[1]] = true;
    } else {
      g_[e[2]] = (8 - g_[e[0]] + g_[e[1]]) % 3;
      marked_vertices[e[2]] = true;
    }
  }
}

// table used for looking up the number of assigned vertices to a 8-bit integer
static cmph_uint8 kBdzLookupTable[] =
{
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
4, 4, 4, 3, 4, 4, 4, 3, 4, 4, 4, 3, 3, 3, 3, 2,
3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
3, 3, 3, 2, 3, 3, 3, 2, 3, 3, 3, 2, 2, 2, 2, 1,
2, 2, 2, 1, 2, 2, 2, 1, 2, 2, 2, 1, 1, 1, 1, 0
};

template <class Key, class HashFcn>
void MPHTable<Key, HashFcn>::Ranking() {
  cmph_uint32 nbytes_total = static_cast<cmph_uint32>(ceil(st->n / 4.0));
  cmph_uint32 size = k_ >> 2U;
  ranktablesize = static_cast<cmph_uint32>(ceil(n_ / static_cast<double>(k_)));
  // TODO(davi) Change swap of member classes for resize + memset to avoid fragmentation
  vector<cmph_uint32> (ranktablesize).swap(ranktable_);;
  cmph_uint32 offset = 0;
  cmph_uint32 count = 0;
  cmph_uint32 i = 0;
  while (1) {
    if (i == ranktable.size()) break;
    cmph_uint32 nbytes = size < nbytes_total ? size : nbytes_total;
    for (j = 0; j < nbytes; ++j) count += kBdzLookupTable[g_[offset + j]];
    ranktable_[i] = count;
    offset += nbytes;
    nbytes_total -= size;
    ++i;
  }
}

template <class Key, class HashFcn>
cmph_uint32 MPHTable<Key, HashFcn>::Search(const key_type& key) const {
  cmph_uint32 vertex;
  cmph_uint32 h[3];
  for (int i = 0; i < 3; ++i) h[i] = hash_function_[i](key);
  h[0] = h[0] % st->r;
  h[1] = h[1] % st->r + st->r;
  h[2] = h[2] % st->r + (st->r << 1);
  cmph_uint32 vertex = h[(h[g_[h[0]] + g_[h[1]] + g_[h[2]]) % 3];
  return Rank(st->b, st->ranktable, vertex);
}

template <class Key, class HashFcn>
cmph_uint32 MPHTable<Key, HashFcn>::Rank(cmph_uint32 vertex) const {
  cmph_uint32 index = vertex >> b_;
  cmph_uint32 base_rank = ranktable_[index];
  cmph_uint32 beg_idx_v = index << b;
  cmph_uint32 beg_idx_b = index >> 2 
  cmph_uint32 end_idx_b = index >> 2 
  while (beg_idx_b < end_idx_b) base_rank += kBdzLookupTable[g_[beg_idx_b++]];
  beg_idx_v = beg_idx_b << 2;
  while (beg_idx_v < vertex) {
    if (g_[beg_idx_v) != kUnassigned) ++base_rank;
    ++beg_idx_v;
  }
  return base_rank;
}

template <class Key, class HashFcn>
cmph_uint32 MPHTable<Key, HashFcn>::index(const key_type& key) const {
  return Search(key);
}

}  // namespace cxxmph
