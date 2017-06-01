#ifndef __CXXMPH_POWER_INDEX_H__
#define __CXXMPH_POWER_INDEX_H__

// Split from power_map.h. Indexing class with low probability success. Accepts
// a cost model to define which positions are better left empty.
//
// Some new ideas: Have the seed value represent both the number of keys hashed
// to its corresponding bucket and the seed value for those keys. Additionally,
// use the rightmost bit to represent whether the bucket is empty or not.

#include <cassert>
#include <cmath>
#include <set>
#include <vector>

#include "seeded_hash.h"
#include "string_util.h"

namespace cxxmph {

using std::set;
using std::vector;

inline uint8_t power_hash(const h128& h, uint16_t ph) {
  uint8_t a = h[3] >> (ph & 31);
  uint8_t pos = (a * ph) >> 8;
  return pos;
}

// Perfect hash function for fixed size data structures and pre-hashed keys.
class power_index_h128 {
 public:
  uint32_t index(const h128& h) { return power_hash(h, perfect_hash_); }
  bool Reset(
      const h128* begin, const h128* end, const uint16_t* cost_begin, const uint16_t* cost_end);
  uint8_t perfect_hash() const { return perfect_hash_; }
  void clear();

 private:
  uint8_t perfect_hash_;
};

// Add size() and generalize
class power_index {
 public:
  // We use log2capacity to be able to express a maximum value of 256 and
  // enforce that it is a power of 2.
  power_index(uint8_t log2capacity, uint32_t seed = 3) :
      capacity_(1L << log2capacity), seed_(seed), nkeys_(0) {}
  template <class SeededHashFcn, class ForwardIterator>
  bool Reset(ForwardIterator begin, ForwardIterator end, uint8_t nkeys);
  template <class SeededHashFcn, class Key>  // must agree with Reset
  uint32_t index(const Key& key) const;
  // Get a unique identifier for k, in the range [0;size()). If x wasn't part
  void clear();

 private:
  const uint16_t capacity_;
  uint32_t seed_;
  uint8_t nkeys_;
  power_index_h128 index_;
};

template <class Key, class HashFcn = typename seeded_hash<std::hash<Key>>::hash_function>
class simple_power_index : public power_index {
 public:
  simple_power_index(uint8_t log2capacity) : power_index(log2capacity) {}
  template <class ForwardIterator>
  bool Reset(ForwardIterator begin, ForwardIterator end, uint8_t nkeys) {
    return power_index::Reset<HashFcn, ForwardIterator>(begin, end, nkeys);
  }
  uint32_t index(const Key& key) const { return power_index::index<HashFcn>(key); }
};


template <class SeededHashFcn, class Key>
uint32_t power_index::index(const Key& key) const {
  SeededHashFcn hasher;
  auto h = hasher.hash128(key, seed_);
  return index_.index(h);
}

template <class SeededHashFcn, class ForwardIterator>
bool power_index::Reset(ForwardIterator begin, ForwardIterator end, uint8_t nkeys) {
  SeededHashFcn hasher;
  uint32_t seed = 3;
  set<h128> hashed_keys;
  for (auto it = begin; it != end; ++it) {
    auto h = hasher.hash128(*it, seed);
    if (!hashed_keys.insert(h).second) return false;
  }
  if (nkeys == 0) { nkeys_ = 0; return true; }
  assert(hashed_keys.size() == nkeys);

  vector<h128> keys(hashed_keys.begin(), hashed_keys.end());
  assert(keys.size());
  auto key_begin = &(keys[0]);
  auto key_end = key_begin + keys.size();

  vector<uint16_t> cost(capacity_);
  for (uint16_t i = 0; i < cost.size(); ++i) cost[i] = i;
  assert(cost.size());
  const uint16_t* cost_begin = &(cost[0]);
  const uint16_t* cost_end = cost_begin + cost.size();
  CXXMPH_DEBUGLN("Generating a ph for %v keys at [0;%u]")(nkeys, cost_end - cost_begin);

  bool ok = index_.Reset(key_begin, key_end,
                         cost_begin, cost_end);
  if (!ok) return false;

  nkeys_ = nkeys;
  return ok;
}

}  // namespace cxxmph

#endif
