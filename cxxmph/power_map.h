#ifndef __CXXMPH_POWER_MAP_H__
#define __CXXMPH_POWER_MAP_H__
// Implementation of the unordered associative mapping interface using a
// a perfect hash function on each bucket.
//
// This class has faster reads than most hash map implementations on all use
// cases, at the expense of significantly higher insertion cost. It has exactly
// 1 byte of overhead per bucket, and 100% occupation on the limit.

#include <algorithm>
#include <iostream>
#include <limits>
#include <set>
#include <unordered_map>
#include <vector>
#include <utility>  // for std::pair

#include "string_util.h"
#include "seeded_hash.h"
#include "mph_bits.h"
#include "hollow_iterator.h"

namespace cxxmph {

using std::cerr;
using std::endl;
using std::pair;
using std::make_pair;
using std::unordered_map;
using std::vector;

// Save on repetitive typing.
#define POWER_MAP_TMPL_SPEC template <class Key, class Data, class HashFcn, class EqualKey, class Alloc>
#define POWER_MAP_CLASS_SPEC power_map<Key, Data, HashFcn, EqualKey, Alloc>
#define POWER_MAP_METHOD_DECL(r, m) POWER_MAP_TMPL_SPEC typename POWER_MAP_CLASS_SPEC::r POWER_MAP_CLASS_SPEC::m
#define POWER_MAP_INLINE_METHOD_DECL(r, m) POWER_MAP_TMPL_SPEC inline typename POWER_MAP_CLASS_SPEC::r POWER_MAP_CLASS_SPEC::m

template <class Key, class Data, class HashFcn = std::hash<Key>, class EqualKey = std::equal_to<Key>, class Alloc = std::allocator<Data> >
class power_map {
 public:
  typedef Key key_type;
  typedef Data data_type;
  typedef pair<Key, Data> value_type;
  typedef HashFcn hasher;
  typedef EqualKey key_equal;

  typedef typename vector<value_type>::pointer pointer;
  typedef typename vector<value_type>::reference reference;
  typedef typename vector<value_type>::const_reference const_reference;
  typedef typename vector<value_type>::size_type size_type;
  typedef typename vector<value_type>::difference_type difference_type;

  typedef is_empty<const vector<value_type>> is_empty_type;
  typedef hollow_iterator_base<typename vector<value_type>::iterator, is_empty_type> iterator;
  typedef hollow_iterator_base<typename vector<value_type>::const_iterator, is_empty_type> const_iterator;

  // For making macros simpler.
  typedef void void_type;
  typedef bool bool_type;
  typedef pair<iterator, bool> insert_return_type;

  power_map();
  ~power_map();

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  size_type size() const;
  bool empty() const;
  void clear();
  void erase(iterator pos);
  void erase(const key_type& k);
  pair<iterator, bool> insert(const value_type& x);
  inline iterator find(const key_type& k);
  inline const_iterator find(const key_type& k) const;
  typedef int32_t my_int32_t;  // help macros
  typedef uint32_t my_uint32_t;  // help macros
  data_type& operator[](const key_type &k);
  const data_type& operator[](const key_type &k) const;

  size_type bucket_count() const { return ph_.size(); }
  void rehash(size_type nbuckets); 

 protected:  // mimicking STL implementation
  EqualKey equal_;

 private:
  static constexpr uint32_t kMaxCost = 1ULL << 16;
  inline uint32_t index(const key_type& k) const;
  inline uint32_t bucket(const key_type& k) const;
  void pack();
  bool fast_insert(const value_type& x);
  bool slow_insert(const value_type& x);
  bool calculate_bucket_ph(
    uint32_t bucket, const h128* new_key,
    uint8_t* ph, vector<h128>* keys);
  bool make_power_index(
    const vector<h128>& keys,
    const vector<uint32_t>& cost,
    uint8_t* ph) const;

   vector<value_type> values_;
   vector<bool> present_;
   vector<uint8_t> ph_;
   vector<uint32_t> cost_;
   typename seeded_hash<HashFcn>::hash_function hasher128_;
   uint32_t seed_;
   uint32_t capacity_;  // values_.size() - 256;
   size_type size_;
};

inline uint8_t power_index(const h128& h, uint16_t ph) {
  uint8_t a = h[3] >> (ph & 31);
  uint8_t pos = (a * ph) >> 8;
  return pos;
}

POWER_MAP_METHOD_DECL(bool_type, make_power_index)(
    const vector<h128>& keys,
    const vector<uint32_t>& cost,
    uint8_t* ph_result) const {
  assert(cost.size() == 256);
  uint32_t best_cost = kMaxCost;
  uint32_t best_ph = 0;
  for (uint16_t ph = 0; ph < 256; ++ph) {
    uint32_t total_cost = 0;
    std::set<uint8_t> used;
    for (const auto& h : keys) {
      auto idx = power_index(h, ph);
      if (!used.insert(idx).second) {
        total_cost = kMaxCost;
        break;
      }
      total_cost += cost[idx];
    }
    if (total_cost < best_cost) {
      best_cost = total_cost;
      best_ph = ph;
    }
  }
  if (best_cost < kMaxCost) {
    CXXMPH_DEBUGLN("Found a ph for %v keys at cost %v")(keys.size(), best_cost);
    if (ph_result) *ph_result = best_ph;
    return true;
  }
  CXXMPH_DEBUGLN("Failed to find a power index for %v keys")(keys.size());
  return false;
}

POWER_MAP_TMPL_SPEC
bool operator==(const POWER_MAP_CLASS_SPEC& lhs, const POWER_MAP_CLASS_SPEC& rhs) {
  return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

POWER_MAP_TMPL_SPEC POWER_MAP_CLASS_SPEC::power_map() {
  clear();
  pack();
}

POWER_MAP_TMPL_SPEC POWER_MAP_CLASS_SPEC::~power_map() {
}

POWER_MAP_METHOD_DECL(insert_return_type, insert)(const value_type& x) {
  CXXMPH_DEBUGLN("Inserting key %v")(x);
  auto it = find(x.first);
  auto it_end = end();
  if (it != it_end) return make_pair(it, false);
  while (!(fast_insert(x) || slow_insert(x))) pack();
  it = find(x.first);
  CXXMPH_DEBUGLN("After insertion found %v at (%v+%v)")(
      x, (hasher128_.hash128(x.first, seed_)[0] % capacity_),
      (it.it_-values_.begin())-hasher128_.hash128(x.first, seed_)[0] % capacity_);
  assert(it != end());
  ++size_;
  return make_pair(it, true);
}

// A fast insert is the tentative of storing a key in its exact location without
// updating power indices.
POWER_MAP_METHOD_DECL(bool_type, fast_insert)(const value_type& x) {
  CXXMPH_DEBUGLN("Trying fast insert")();
  auto idx = index(x.first);
  if (ph_[idx] == 0 && !present_[idx]) {
    ph_[idx] = 1;
    present_[idx] = true;
    assert(cost_[idx] == 0);
    cost_[idx] = 1;
    values_[idx] = x;
    return true;
  }
  CXXMPH_DEBUGLN("Index %v taken by key %v (%v+%v)")(
      idx, values_[idx], bucket(values_[idx].first),
      idx - bucket(values_[idx].first));
  return false;
}

POWER_MAP_METHOD_DECL(bool_type, calculate_bucket_ph)(
    uint32_t b, const h128* new_key, uint8_t* ph, vector<h128>* keys) {
  // Get keys and cost
  uint8_t nkeys = 0;
  h128 keys_array[256];
  vector<uint32_t> cost(cost_.begin() + b,
                        cost_.begin() + b + 256);;
  for (int i = 0; i < 256; ++i) {
    if (!present_[b + i]) continue;
    auto occupying_bucket = bucket(values_[b + i].first);
    if (occupying_bucket == b) {
      cost[i] = 0;
      auto h = hasher128_.hash128(values_[b+i].first, seed_);
      keys_array[nkeys++] = h;
    }
  }
  if (new_key) keys_array[nkeys++] = *new_key;
  vector<h128>(keys_array, keys_array + nkeys).swap(*keys);
  CXXMPH_DEBUGLN("Making power index for %v keys at bucket %v")(
      keys->size(), b);
  bool r = make_power_index(*keys, cost, ph);
  return r;
}

POWER_MAP_METHOD_DECL(bool_type, slow_insert)(const value_type& x) {
  CXXMPH_DEBUGLN("Trying slow insert");
  // Find the ph with smallest cost for this insertion
  auto b = bucket(x.first);
  uint8_t ph;
  auto h = hasher128_.hash128(x.first, seed_);
  vector<h128> keys;
  if (!calculate_bucket_ph(b, &h, &ph, &keys)) return false;

  // Update cost vector and rollback for cost vector if insertion fails
  unordered_map<uint32_t, uint32_t> cost_rollback;
  for (auto hit = keys.begin(); hit != keys.end(); ++hit) {
    auto idx = b + power_index(*hit, ph);
    cost_rollback[idx] = cost_[idx];
    CXXMPH_DEBUGLN("Reserving idx (%v+%v) for key %v:%v")(b, idx-b, (*hit)[0], (*hit)[3]);
    cost_[idx] = kMaxCost;
  }

  // Remove and reinsert under new cost vector any key using a bucket needed by
  // the generated ph
  for (auto hit = keys.begin(); hit != keys.end(); ++hit) {
    auto idx = b + power_index(*hit, ph);
    if (!present_[idx]) continue;
    auto occupying_bucket = bucket(values_[idx].first);
    if (occupying_bucket == b) continue;  // will be redistributed
    auto y = values_[idx];
    CXXMPH_DEBUGLN("Dumping key %v from idx (%v+%v)")(y.first, b, idx-b);
    values_[idx] = x;
    bool pushed = slow_insert(y);
    if (pushed) {
      values_[idx] = y;
      continue;
    }
    for (auto cit = cost_rollback.begin(); cit != cost_rollback.end(); ++cit) {
      assert(cost_[cit->first] == kMaxCost);
      cost_[cit->first] = cit->second;
    }
    CXXMPH_DEBUGLN("Failed to dump key. Rolled back and gave up.")();
    return false;
  }

  // Redistributes the keys for this new ph
  for (auto hit = keys.begin(); hit != keys.end(); ++hit) {
    if (*hit == h) continue;
    auto old_idx = b + power_index(*hit, ph_[b]);
    auto idx = b + power_index(*hit, ph);
    if (present_[idx]) {
      CXXMPH_DEBUGLN("Swapping val %v at (%v+%v) with val %v at (%v+%v)")(
          values_[idx], b, idx-b, values_[old_idx], b, old_idx-b);
      swap(values_[old_idx], values_[idx]);
    } else {
      CXXMPH_DEBUGLN("Moving val %v from (%v+%v) to (%v+%v)")(
          values_[old_idx], b, old_idx-b, b, idx-b);
      values_[idx] = values_[old_idx];
      present_[idx] = true;
      values_[old_idx] = value_type();
      present_[old_idx] = false;
      cost_[old_idx] = 0;
    }
    cost_[idx] = keys.size() * keys.size();
  }

  // And finally inserts
  auto idx = b + power_index(h, ph);
  CXXMPH_DEBUGLN("Final idx for key %v is (%v+%u)")(
       x.first, b , power_index(h, ph));
  assert(!present_[idx]);
  values_[idx] = x;
  present_[idx] = true;
  cost_[idx] = keys.size() * keys.size();
  ph_[b] = ph;
  return true;
}

POWER_MAP_METHOD_DECL(void_type, pack)() {
}

POWER_MAP_METHOD_DECL(iterator, begin)() { return make_hollow(&values_, &present_, values_.begin()); }
POWER_MAP_METHOD_DECL(iterator, end)() { return make_solid(&values_, &present_, values_.end()); }
POWER_MAP_METHOD_DECL(const_iterator, begin)() const { return make_hollow(&values_, &present_, values_.begin()); }
POWER_MAP_METHOD_DECL(const_iterator, end)() const { return make_solid(&values_, &present_, values_.end()); }
POWER_MAP_METHOD_DECL(bool_type, empty)() const { return size_ == 0; }
POWER_MAP_METHOD_DECL(size_type, size)() const { return size_; }

POWER_MAP_METHOD_DECL(void_type, clear)() {
  decltype(values_)(256+8).swap(values_);
  decltype(present_)(256+8).swap(present_);
  decltype(ph_)(256+8).swap(ph_);
  decltype(cost_)(256+8).swap(cost_);
  capacity_ = 8;
  size_ = 0;
  seed_ = random();
  cerr << "seed initialized to " << seed_ << endl;
}

POWER_MAP_METHOD_DECL(void_type, erase)(iterator pos) {
  present_[pos.it_ - begin().it_] = false;
  *pos = value_type();
  --size_;
}

POWER_MAP_METHOD_DECL(void_type, erase)(const key_type& k) {
  iterator it = find(k);
  if (it == end()) return;
  erase(it);
}

POWER_MAP_INLINE_METHOD_DECL(const_iterator, find)(const key_type& k) const {
  auto idx = index(k);
  auto vit = values_.begin() + idx;
  if (!present_[idx] || vit->first != k) return end();
  return make_solid(&values_, &present_, vit);;
}

POWER_MAP_INLINE_METHOD_DECL(iterator, find)(const key_type& k) {
  auto idx = index(k);
  auto vit = values_.begin() + idx;
  key_type fk = vit->first;
  if (!present_[idx] || !(fk == k)) return end();
  return make_solid(&values_, &present_, vit);;
}

POWER_MAP_INLINE_METHOD_DECL(my_uint32_t, index)(const key_type& k) const {
  auto h = hasher128_.hash128(k, seed_);
  auto b = h[0] % (capacity_);
  auto ph = ph_[b];
  auto idx = b + power_index(h, ph);
  return idx;
}

POWER_MAP_INLINE_METHOD_DECL(my_uint32_t, bucket)(const key_type& k) const {
  auto h = hasher128_.hash128(k, seed_);
  return h[0] % (capacity_);
}

POWER_MAP_METHOD_DECL(data_type&, operator[])(const key_type& k) {
  return insert(make_pair(k, data_type())).first->second;
}
POWER_MAP_METHOD_DECL(void_type, rehash)(size_type nbuckets) {
  pack();
  vector<value_type>(values_.begin(), values_.end()).swap(values_);
  vector<bool>(present_.begin(), present_.end()).swap(present_);
  decltype(ph_)(ph_.begin(), ph_.end()).swap(ph_);
}

}  // namespace cxxmph

#endif  // __CXXMPH_POWER_MAP_H__
