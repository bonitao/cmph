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
using std::pair;

inline uint16_t power_byte_hash(uint32_t value, uint8_t seed) {
  // Secret sauce. The higher the seed value, the more bits 
  // we use from the hash, allowing for more spread.
  return value & (seed * seed);
}

class power_byte_index {
 public:
  typedef h128 key_type;
  typedef uint32_t data_type;
  typedef pair<h128, uint32_t> value_type;
  typedef pair<h128, uint32_t> value_type;
  typedef std::equal<h128> key_equal;
  typedef std::hash<h128> hasher;

  typedef typename vector<value_type>::pointer pointer;
  typedef typename vector<value_type>::reference reference;
  typedef typename vector<value_type>::const_reference const_reference;
  typedef typename vector<value_type>::size_type size_type;
  typedef typename vector<value_type>::difference_type difference_type;

  bool Reset(const h128* begin, const h128* end)
  uint32_t index(const h128& h) const { 
    auto bucket = h[0] & (capacity_ - 1);
    auto seed = index_[bucket];
    return bucket + power_byte_hash(h[3], seed) 
  }
  uint32_t size() const { return size_; }
  void clear();

  void erase(const h128* k) { present_[index(k)] = false; }
  void rehash(size_type nbuckets); 

 private:
  void rehash(size_type nbuckets, const vector<value_type>& extra_values); 
  vector<uint8_t> index_;
  vector<bool> present_;
  vector<value_type> values_;
  size_type size_;
  size_type capacity_;
};

bool power_byte_index::Reset(const h128* begin, const h128* end) {
  // Allocate new data structures
  size_type capacity = (end - begin)*2;
  vector<uint8_t> index(capacity_ + UINT16_MAX);
  vector<bool> present(capacity_ + UINT16_MAX);
  vector<value_type> values(capacity_ + UINT16_MAX);
  size_type size = 0;
  // Swap with existing ones for atomic reset.
  swap(capacity, capacity_); 
  index_.swap(index);
  present_.swap(present);
  values_.swap(values);
  swap(size, size_);
  // Try to insert all keys in the now empty hash table
  bool failed = false;
  for (auto it = begin; it != end; ++it) {
    if (!insert(*it)) {
      failed = true;
      break;
    }
  }
  // If failed, roll back and return false
  if (failed) {
    swap(capacity_, capacity);
    index_.swap(index);
    swap(size_, size);
    present_.swap(present);
    values_.swap(values);
    return false;
  }
  return true;
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
