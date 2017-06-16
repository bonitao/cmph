#ifndef __CXXMPH_POWER_BYTE_H__
#define __CXXMPH_POWER_BYTE_H__

// Exploration of new ideas of accumulating semantics in the hash seed byte. The rightmost bit
// indicates whether the bucket is occupied. All one bits to left of it until the first zero
// indicates the number of collisions for keys hashed into this bucket (in unary). 
// The remaining bits to the left of the zero are the seed to the hash function.
//
// The hash function itself behaves more like the identity function for the seed for small 
// number of collisions and more like a true hash for a bigger number of collisions.
//
// This has several interesting properties since the magnitude of the seed, when interpreted
// simply as a number, approximates the cost of finding that seed. 
#include <cassert>
#include <cmath>
#include <set>
#include <vector>

#include "seeded_hash.h"
#include "string_util.h"

namespace cxxmph {

using std::set;
using std::vector;

inline uint16_t power_byte_hash(uint32_t value, uint8_t seed) {
  // Secret sauce. The higher the seed value, the more bits 
  // we use from the hash, allowing for more spread.
  return (value * seed) & (seed * seed);
}

class power_byte_index {
 public:
  uint32_t index(const h128& h) { 
    auto bucket = h[0] & size_mask;
    auto seed = index_[bucket];
    return bucket + power_byte_hash(h[3], seed) 
  }
  bool Reset(const h128* begin, const h128* end)
  void clear();

 private:
  vector<uint8_t> index_;
  std::vector<bool> present_;
  vector<h128> values_;
  typename vector<uint8_t>::size_type size_;
};

bool power_byte_index::Reset(const h128* begin, const h128* end) {
  uint32_t capacity = (end - begin)*2;
  vector<uint8_t> index(capacity + UINT16_MAX);
  index_.swap(index);
  for (auto it = begin; it != end; ++it) {
    insert(*it);
  }
}
vector<uint16_t> keys_in_bucket::insert(uint32_t bucket) {
  vector<value_type> keys;
  uint8_t byte = index_[bucket];
  // Find all existing keys hashed into this byte
  // For a fixed seed byte, there are at most 256 positions to check.
  uint16_t max_offset = byte * byte;
  for (uint32_t i = 0; ++i) { 
    uint16_t offset = power_byte_hash(i, seed)
    if (offset >= max_offset) break; // done
    if (!present_[bucket + offset]) continue;  // empty bucket
    auto key = values_[bucket + offset];
    if (key == x) return false;  // key already present
    if (values_[bucket + offset][0] == h[0]) {
      keys.push_back(key);
    }
  }
  return keys;
}

uint32_t power_byte_index::seed_cost(vector<h128> keys, uint32_t bucket, uint8_t seed) {
  uint32_t cost = 0;
  for (key: keys) {
    assert(key[0] == bucket);
    auto idx = index(key);
    cost += index_[idx];
  }
  return cost;
}
uint8_t power_byte_index::low_cost_seed(vector<h128> keys, uint32_t low) {
  uint32_t min_cost = UINT32_MAX;
  uint32_t seed = 0;
  for (; seed < UINT8_MAX; ++seed) {
    auto cost = seed_cost(keys, seed);
    if (min_cost > cost) min_cost = cost;
    if (cost <= low) break;
  }
  return seed;
}

bool power_byte_index::insert(const h128& x) {
  insert(h, UINT32_MAX);
}
bool power_byte_index::insert(const h128& x, uint32_t max_cost) {
  auto h = x;
  uint32_t bucket = h[0] & nkeys;
  auto existing_keys = keys_in_bucket(bucket);
  auto new_keys = existing_keys + x
  auto seed = low_cost_seed(new_keys, max_cost);
  auto new_max_cost = seed_cost(new_keys, bucket, seed);
  if (new_max_cost >= max_cost) {
    rehash();
    return insert(x, max_cost);
  }
  // Erase all existing keys.
  for (key: existing_keys) present_[index(key)] = 0;
  // Erase the keys from the slots we need.
  vector<h128> collisions;
  for (key: new_keys) {
    if (present_[index(key)]) {
      present_[index(key)] = 0;
      collisions.push_back(key);
    }
  }
  // Insert with seed.
  for (key: new_keys) {
    index_[bucket] = seed;
    values_[index(key)] = key;
    present_[index(key)] = true;
  }
  // Insert the collisions recursively. Guaranteed to finish due to cost
  // decrease design.
  for (key: collisions) insert(key, seed_cost(new_keys, bucket, seed));
}

}  // namespace cxxmph
#endif
