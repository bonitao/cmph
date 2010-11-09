#ifndef __CXXMPH_MPHTABLE_H__
#define __CXXMPH_MPHTABLE_H__

// Minimal perfect hash abstraction implementing the BDZ algorithm

#include <cassert>
#include <cmath>
#include <unordered_map>  // for std::hash
#include <vector>

#include <iostream>

using std::cerr;
using std::endl;

#include "cmph_hash_function.h"
#include "trigraph.h"

namespace cxxmph {

class MPHTable {
 public:
  MPHTable(double c = 1.23, cmph_uint8 b = 7) :
      c_(c), b_(b), m_(0), n_(0), k_(0), r_(0) { }
  ~MPHTable() {}

  template <class SeededHashFcn, class ForwardIterator>
  bool Reset(ForwardIterator begin, ForwardIterator end);
  template <class SeededHashFcn, class Key>  // must agree with Reset
  cmph_uint32 index(const Key& x) const;
  cmph_uint32 size() const { return m_; }
  void clear();

 private:
  template <class SeededHashFcn, class ForwardIterator>
  bool Mapping(ForwardIterator begin, ForwardIterator end,
               std::vector<TriGraph::Edge>* edges,
               std::vector<cmph_uint32>* queue);
  bool GenerateQueue(TriGraph* graph, std::vector<cmph_uint32>* queue);
  void Assigning(const std::vector<TriGraph::Edge>& edges,
                 const std::vector<cmph_uint32>& queue);
  void Ranking();
  cmph_uint32 Rank(cmph_uint32 vertex) const;

  // Algorithm parameters
  double c_;  // Number of bits per key (? is it right)
  cmph_uint8 b_;  // Number of bits of the kth index in the ranktable

  // Values used during generation
  cmph_uint32 m_;  // edges count
  cmph_uint32 n_;  // vertex count
  cmph_uint32 k_;  // kth index in ranktable, $k = log_2(n=3r)\varepsilon$

  // Values used during search

  // Partition vertex count, derived from c parameter.
  cmph_uint32 r_;
  // The array containing the minimal perfect hash function graph.
  std::vector<cmph_uint8> g_;
  // The table used for the rank step of the minimal perfect hash function
  std::vector<cmph_uint32> ranktable_;
  // The selected hash seed triplet for finding the edges in the minimal
  // perfect hash function graph.
  cmph_uint32 hash_seed_[3];

  static const cmph_uint8 valuemask[];
  static void set_2bit_value(std::vector<cmph_uint8> *d, cmph_uint32 i, cmph_uint8 v) {
    (*d)[(i >> 2)] &= (v << ((i & 3) << 1)) | valuemask[i & 3];
  }
  static cmph_uint32 get_2bit_value(const std::vector<cmph_uint8>& d, cmph_uint32 i) {
    return (d[(i >> 2)] >> ((i & 3) << 1)) & 3;
  }

  
};

// Template method needs to go in the header file.
template <class SeededHashFcn, class ForwardIterator>
bool MPHTable::Reset(ForwardIterator begin, ForwardIterator end) {
  m_ = end - begin;
  r_ = static_cast<cmph_uint32>(ceil((c_*m_)/3));
  if ((r_ % 2) == 0) r_ += 1;
  n_ = 3*r_;
  k_ = 1U << b_;

  cerr << "m " << m_ << " n " << n_ << " r " << r_ << endl;

  int iterations = 10;
  std::vector<TriGraph::Edge> edges;
  std::vector<cmph_uint32> queue;
  while (1) {
    cerr << "Iterations missing: " << iterations << endl;
    for (int i = 0; i < 3; ++i) hash_seed_[i] = random() % m_;
    // for (int i = 0; i < 3; ++i) hash_seed_[i] = random() + i;
    if (Mapping<SeededHashFcn>(begin, end, &edges, &queue)) break;
    else --iterations;
    if (iterations == 0) break;
  }
  if (iterations == 0) return false;
  Assigning(edges, queue);
  std::vector<TriGraph::Edge>().swap(edges);
  Ranking();
  return true;
}

template <class SeededHashFcn, class ForwardIterator>
bool MPHTable::Mapping(
    ForwardIterator begin, ForwardIterator end,
    std::vector<TriGraph::Edge>* edges, std::vector<cmph_uint32>* queue) {
  TriGraph graph(n_, m_);
  for (ForwardIterator it = begin; it != end; ++it) { 
    cmph_uint32 h[3];
    for (int i = 0; i < 3; ++i) h[i] = SeededHashFcn()(*it, hash_seed_[i]);
    cmph_uint32 v0 = h[0] % r_;
    cmph_uint32 v1 = h[1] % r_ + r_;
    cmph_uint32 v2 = h[2] % r_ + (r_ << 1);
    // cerr << "Key: " << *it << " edge " <<  it - begin << " (" << v0 << "," << v1 << "," << v2 << ")" << endl;
    graph.AddEdge(TriGraph::Edge(v0, v1, v2));
  }
  if (GenerateQueue(&graph, queue)) {
     graph.ExtractEdgesAndClear(edges);
     return true;
  }
  return false;
}

template <class SeededHashFcn, class Key>
cmph_uint32 MPHTable::index(const Key& key) const {
  cmph_uint32 h[3];
  for (int i = 0; i < 3; ++i) h[i] = SeededHashFcn()(key, hash_seed_[i]);
  h[0] = h[0] % r_;
  h[1] = h[1] % r_ + r_;
  h[2] = h[2] % r_ + (r_ << 1);
  assert(g_.size());
  //cerr << "g_.size() " << g_.size() << " h0 >> 2 " << (h[0] >> 2) << endl;
  assert((h[0] >> 2) <g_.size());
  assert((h[1] >> 2) <g_.size());
  assert((h[2] >> 2) <g_.size());
  cmph_uint32 vertex = h[(get_2bit_value(g_, h[0]) + get_2bit_value(g_, h[1]) + get_2bit_value(g_, h[2])) % 3];
  // cerr << "Search found vertex " << vertex << endl;
  return Rank(vertex);
}

template <class Key, class HashFcn = typename OptimizedSeededHashFunction<std::hash<Key> >::hash_function>
class SimpleMPHTable : public MPHTable {
 public:
  template <class ForwardIterator>
  bool Reset(ForwardIterator begin, ForwardIterator end) {
    return MPHTable::Reset<HashFcn>(begin, end);
  }
  cmph_uint32 index(const Key& key) { return MPHTable::index<HashFcn>(key); }
};

}  // namespace cxxmph

#endif // __CXXMPH_MPHTABLE_H__
