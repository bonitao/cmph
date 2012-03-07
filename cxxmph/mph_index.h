#ifndef __CXXMPH_MPH_INDEX_H__
#define __CXXMPH_MPH_INDEX_H__

// Minimal perfect hash abstraction implementing the BDZ algorithm
//
// This is a data structure that given a set of known keys S, will create a
// mapping from S to [0..|S|). The class is informed about S through the Reset
// method and the mapping is queried by calling index(key).
//
// This is a pretty uncommon data structure, and if you application has a real
// use case for it, chances are that it is a real win. If all you are doing is
// a straightforward implementation of an in-memory associative mapping data
// structure (e.g., mph_map.h), then it will probably be slower, since that the
// evaluation of index() is typically slower than the total cost of running a
// traditional hash function over a key and doing 2-3 conflict resolutions on
// 100byte-ish strings.
//
// Notes:
//
// Most users can use the SimpleMPHIndex wrapper instead of the MPHIndex which
// have confusing template parameters.
// This class only implements a minimal perfect hash function, it does not
// implement an associative mapping data structure.

#include <stdint.h>

#include <cassert>
#include <cmath>
#include <unordered_map>  // for std::hash
#include <vector>

#include <iostream>

using std::cerr;
using std::endl;

#include "seeded_hash.h"
#include "trigraph.h"

namespace cxxmph {

class MPHIndex {
 public:
  MPHIndex(double c = 1.23, uint8_t b = 7) :
      c_(c), b_(b), m_(0), n_(0), k_(0), r_(0),
      g_(NULL), g_size_(0), ranktable_(NULL), ranktable_size_(0),
      deserialized_(false) { }
  ~MPHIndex();

  template <class SeededHashFcn, class ForwardIterator>
  bool Reset(ForwardIterator begin, ForwardIterator end);
  template <class SeededHashFcn, class Key>  // must agree with Reset
  // Get a unique identifier for k, in the range [0;size()). If x wasn't part
  // of the input in the last Reset call, returns a random value.
  uint32_t index(const Key& x) const;
  uint32_t size() const { return m_; }
  void clear();

  // Advanced users functions. Please avoid unless you know what you are doing.
  uint32_t perfect_hash_size() const { return n_; }
  template <class SeededHashFcn, class Key>  // must agree with Reset
  uint32_t perfect_hash(const Key& x) const;
  template <class SeededHashFcn, class Key>  // must agree with Reset
  uint32_t minimal_perfect_hash(const Key& x) const;

  // Serialization for mmap usage - not tested well, ping me if you care. 
  // Serialized tables are not guaranteed to work across versions or different
  // endianness (although they could easily be made to be).
  uint32_t serialize_bytes_needed() const;
  void serialize(char *memory) const;
  bool deserialize(const char* serialized_memory);

 private:
  template <class SeededHashFcn, class ForwardIterator>
  bool Mapping(ForwardIterator begin, ForwardIterator end,
               std::vector<TriGraph::Edge>* edges,
               std::vector<uint32_t>* queue);
  bool GenerateQueue(TriGraph* graph, std::vector<uint32_t>* queue);
  void Assigning(const std::vector<TriGraph::Edge>& edges,
                 const std::vector<uint32_t>& queue);
  void Ranking();
  uint32_t Rank(uint32_t vertex) const;

  // Algorithm parameters
  double c_;  // Number of bits per key (? is it right)
  uint8_t b_;  // Number of bits of the kth index in the ranktable

  // Values used during generation
  uint32_t m_;  // edges count
  uint32_t n_;  // vertex count
  uint32_t k_;  // kth index in ranktable, $k = log_2(n=3r)\varepsilon$

  // Values used during search

  // Partition vertex count, derived from c parameter.
  uint32_t r_;
  // The array containing the minimal perfect hash function graph. Do not use
  // c++ vector to make mmap based backing easier.
  const uint8_t* g_;
  uint32_t g_size_;
  // The table used for the rank step of the minimal perfect hash function
  const uint32_t* ranktable_;
  uint32_t ranktable_size_;
  // The selected hash seed triplet for finding the edges in the minimal
  // perfect hash function graph.
  uint32_t hash_seed_[3];

  bool deserialized_;

  static const uint8_t valuemask[];
  static void set_2bit_value(uint8_t *d, uint32_t i, uint8_t v) {
    d[(i >> 2)] &= ((v << ((i & 3) << 1)) | valuemask[i & 3]);
  }
  static uint32_t get_2bit_value(const uint8_t* d, uint32_t i) {
    return (d[(i >> 2)] >> (((i & 3) << 1)) & 3);
  }

  
};

// Template method needs to go in the header file.
template <class SeededHashFcn, class ForwardIterator>
bool MPHIndex::Reset(ForwardIterator begin, ForwardIterator end) {
  if (end == begin) {
    clear();
    return true;
  }
  m_ = end - begin;
  r_ = static_cast<uint32_t>(ceil((c_*m_)/3));
  if ((r_ % 2) == 0) r_ += 1;
  n_ = 3*r_;
  k_ = 1U << b_;

  // cerr << "m " << m_ << " n " << n_ << " r " << r_ << endl;

  int iterations = 1000;
  std::vector<TriGraph::Edge> edges;
  std::vector<uint32_t> queue;
  while (1) {
    // cerr << "Iterations missing: " << iterations << endl;
    for (int i = 0; i < 3; ++i) hash_seed_[i] = random();
    if (Mapping<SeededHashFcn>(begin, end, &edges, &queue)) break;
    else --iterations;
    if (iterations == 0) break;
  }
  if (iterations == 0) return false;
  Assigning(edges, queue);
  std::vector<TriGraph::Edge>().swap(edges);
  Ranking();
  deserialized_ = false;
  return true;
}

template <class SeededHashFcn, class ForwardIterator>
bool MPHIndex::Mapping(
    ForwardIterator begin, ForwardIterator end,
    std::vector<TriGraph::Edge>* edges, std::vector<uint32_t>* queue) {
  TriGraph graph(n_, m_);
  for (ForwardIterator it = begin; it != end; ++it) { 
    uint32_t h[3];
    for (int i = 0; i < 3; ++i) h[i] = SeededHashFcn()(*it, hash_seed_[i]);
    uint32_t v0 = h[0] % r_;
    uint32_t v1 = h[1] % r_ + r_;
    uint32_t v2 = h[2] % r_ + (r_ << 1);
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
uint32_t MPHIndex::perfect_hash(const Key& key) const {
  uint32_t h[3];
  for (int i = 0; i < 3; ++i) h[i] = SeededHashFcn()(key, hash_seed_[i]);
  assert(r_);
  h[0] = h[0] % r_;
  h[1] = h[1] % r_ + r_;
  h[2] = h[2] % r_ + (r_ << 1);
  assert(g_size_);
  // cerr << "g_.size() " << g_size_ << " h0 >> 2 " << (h[0] >> 2) << endl;
  assert((h[0] >> 2) <g_size_);
  assert((h[1] >> 2) <g_size_);
  assert((h[2] >> 2) <g_size_);
  uint32_t vertex = h[(get_2bit_value(g_, h[0]) + get_2bit_value(g_, h[1]) + get_2bit_value(g_, h[2])) % 3];
  return vertex;
}
template <class SeededHashFcn, class Key>
uint32_t MPHIndex::minimal_perfect_hash(const Key& key) const {
  return Rank(perfect_hash<SeededHashFcn, Key>(key));
}

template <class SeededHashFcn, class Key>
uint32_t MPHIndex::index(const Key& key) const {
  return minimal_perfect_hash<SeededHashFcn, Key>(key);
}

// Simple wrapper around MPHIndex to simplify calling code. Please refer to the
// MPHIndex class for documentation.
template <class Key, class HashFcn = typename seeded_hash<std::hash<Key> >::hash_function>
class SimpleMPHIndex : public MPHIndex {
 public:
  template <class ForwardIterator>
  bool Reset(ForwardIterator begin, ForwardIterator end) {
    return MPHIndex::Reset<HashFcn>(begin, end);
  }
  uint32_t index(const Key& key) const { return MPHIndex::index<HashFcn>(key); }
  uint32_t perfect_hash(const Key& key) const { return MPHIndex::perfect_hash<HashFcn>(key); }
  uint32_t minimal_perfect_hash(const Key& key) const { return MPHIndex::minimal_perfect_hash<HashFcn>(key); }
};

}  // namespace cxxmph

#endif // __CXXMPH_MPH_INDEX_H__
