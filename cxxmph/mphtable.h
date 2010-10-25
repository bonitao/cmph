#ifndef __CXXMPH_MPHTABLE_H__
#define __CXXMPH_MPHTABLE_H__

// Minimal perfect hash abstraction implementing the BDZ algorithm

#include <vector>

#include "randomly_seeded_hash.h"
#include "stringpiece.h"
#include "trigraph.h"

namespace cxxmph {

template <class Key, class NewRandomlySeededHashFcn = RandomlySeededMurmur2>
class MPHTable {
 public:
  typedef Key key_type;
  typedef NewRandomlySeededHashFcn hasher;
  MPHTable(double c = 1.23, cmph_uint8 b = 7) : c_(c), b_(b) { }
  ~MPHTable();

  template <class ForwardIterator>
  bool Reset(ForwardIterator begin, ForwardIterator end);
  cmph_uint32 index(const key_type& x) const;

 private:
  template <class ForwardIterator>
  bool Mapping(ForwardIterator begin, ForwardIterator end,
               vector<Edge>* edges, vector<cmph_uint32> queue);
  bool GenerateQueue(TriGraph* graph, vector<cmph_uint32>* queue);
  void Assigning(TriGraph* graph_builder, Queue* queue);
  void Ranking(TriGraph* graph_builder, Queue* queue);
  cmph_uint32 Search(const StringPiece& key);
  cmph_uint32 Rank(const StringPiece& key);

  // Algorithm parameters
  cmph_uint8 b_;  // Number of bits of the kth index in the ranktable
  double c_;  // Number of bits per key (? is it right)

  // Values used during generation
  cmph_uint32 m_;  // edges count
  cmph_uint32 n_;  // vertex count
  cmph_uint32 k_  // kth index in ranktable, $k = log_2(n=3r)\varepsilon$

  // Values used during search

  // Partition vertex count, derived from c parameter.
  cmph_uint32 r_;
  // The array containing the minimal perfect hash function graph.
  std::vector<cmph_uint8> g_;
  // The table used for the rank step of the minimal perfect hash function
  std::vector<cmph_uint32> ranktable_;
  // The selected hash function triplet for finding the edges in the minimal
  // perfect hash function graph.
  hasher hash_function_[3];
  
};

}  // namespace cxxmph

#define // __CXXMPH_MPHTABLE_H__
