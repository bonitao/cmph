#ifndef __CXXMPH_MPHTABLE_H__
#define __CXXMPH_MPHTABLE_H__

// Minimal perfect hash abstraction implementing the BDZ algorithm

#include <cmath>
#include <vector>

#include <iostream>

using std::cerr;
using std::endl;

#include "randomly_seeded_hash.h"
#include "stringpiece.h"
#include "trigraph.h"

namespace cxxmph {

class MPHTable {
 public:
  // This class could be a template for both key type and hash function, but we
  // chose to go with simplicity.
  typedef StringPiece key_type;
  typedef RandomlySeededHashFunction<Murmur2StringPiece> hasher_type;

  MPHTable(double c = 1.23, cmph_uint8 b = 7) :
      c_(c), b_(b), m_(0), n_(0), k_(0), r_(0) { }
  ~MPHTable() {}

  template <class ForwardIterator>
  bool Reset(ForwardIterator begin, ForwardIterator end);
  cmph_uint32 index(const key_type& x) const;
  cmph_uint32 size() const { return m_; }
  void clear();

 private:
  template <class ForwardIterator>
  bool Mapping(ForwardIterator begin, ForwardIterator end,
               std::vector<TriGraph::Edge>* edges,
               std::vector<cmph_uint32>* queue);
  bool GenerateQueue(TriGraph* graph, std::vector<cmph_uint32>* queue);
  void Assigning(const std::vector<TriGraph::Edge>& edges,
                 const std::vector<cmph_uint32>& queue);
  void Ranking();
  cmph_uint32 Search(const key_type& key) const;
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
  // The selected hash function triplet for finding the edges in the minimal
  // perfect hash function graph.
  hasher_type hash_function_[3];
  
};

// Template method needs to go in the header file.
template <class ForwardIterator>
bool MPHTable::Reset(ForwardIterator begin, ForwardIterator end) {
  m_ = end - begin;
  r_ = static_cast<cmph_uint32>(ceil((c_*m_)/3));
  if ((r_ % 2) == 0) r_ += 1;
  n_ = 3*r_;
  k_ = 1U << b_;

  cerr << "m " << m_ << " n " << n_ << " r " << r_ << endl;

  int iterations = 1000;
  std::vector<TriGraph::Edge> edges;
  std::vector<cmph_uint32> queue;
  while (1) {
    cerr << "Iterations missing: " << iterations << endl;
    for (int i = 0; i < 3; ++i) hash_function_[i] = hasher_type();
    // hash_function_[0] = hasher_type();
    cerr << "Seed: " << hash_function_[0].seed << endl;
    if (Mapping(begin, end, &edges, &queue)) break;
    else --iterations;
    if (iterations == 0) break;
  }
  if (iterations == 0) return false;
  Assigning(edges, queue);
  std::vector<TriGraph::Edge>().swap(edges);
  Ranking();
  return true;
}

template <class ForwardIterator>
bool MPHTable::Mapping(
    ForwardIterator begin, ForwardIterator end,
    std::vector<TriGraph::Edge>* edges, std::vector<cmph_uint32>* queue) {
  TriGraph graph(n_, m_);
  for (ForwardIterator it = begin; it != end; ++it) { 
    cmph_uint32 h[3];
    for (int i = 0; i < 3; ++i) h[i] = hash_function_[i](*it);
    // hash_function_[0](*it, h);
    cmph_uint32 v0 = h[0] % r_;
    cmph_uint32 v1 = h[1] % r_ + r_;
    cmph_uint32 v2 = h[2] % r_ + (r_ << 1);
    cerr << "Key: " << *it << " edge " <<  it - begin << " (" << v0 << "," << v1 << "," << v2 << ")" << endl;
    graph.AddEdge(TriGraph::Edge(v0, v1, v2));
  }
  if (GenerateQueue(&graph, queue)) {
     graph.ExtractEdgesAndClear(edges);
     return true;
  }
  return false;
}

}  // namespace cxxmph

#endif // __CXXMPH_MPHTABLE_H__
