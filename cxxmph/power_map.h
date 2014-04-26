#ifndef __CXXMPH_POWER_MAP_H__
#define __CXXMPH_POWER_MAP_H__
// Implementation of the unordered associative mapping interface using a
// a perfect hash function on each bucket.
//
// This class has faster reads than most hash map implementations on all use
// cases, at the expense of significantly higher insertion cost. It has exactly
// 1 byte of overhead per bucket, and 100% occupation on the limit.
//
// Update from myself: the idea is that there is one bucket per key. Instead of
// a bucket holding an offset, it holds a perfect hash function, so collisions
// within the bucket can be resolved with it. Even if there are too many
// collisions for a single bucket, re-bucketing through the extra bytes in h
// could prove to be even more powerful.

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
#include "power_index.h"
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
  typedef power_map<Key, Data, HashFcn, EqualKey, Alloc> self_type;

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
  data_type& operator[](const key_type& k);
  const data_type& operator[](const key_type& k) const;

  size_type bucket_count() const { return ph_.size(); }
  void rehash(size_type nbuckets);
  void swap(self_type& rhs);

  string l1_pressure() const;
  string l2_pressure() const;


 protected:  // mimicking STL implementation
  EqualKey equal_;

 private:
  static constexpr uint32_t kMaxCost = 1ULL << 16;
  void pack();
  bool Reset(const_iterator it, const_iterator end);
  inline uint32_t index(const key_type& k) const;
  inline uint32_t bucket(const key_type& k) const;
  bool fast_insert(const value_type& x);
  bool slow_insert(const value_type& x);
  bool calculate_bucket_ph(
      uint32_t bucket, const h128* new_key, uint8_t *ph) const;
  // Input contains all keys in the 256 positions adjacent to the bucket.
  bool make_power_index(
    uint32_t b,
    pair<bool, h128> bucket_view[256],
    uint8_t* result_ph) const;

  vector<value_type> values_;
  vector<bool> present_;
  vector<uint8_t> ph_;
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
    uint32_t b,
    pair<bool, h128> bucket_view[256],
    uint8_t* result_ph) const {
  // Collect keys in this bucket and free space used by them.
  h128 bucket_keys[256];
  uint16_t cost[256];
  uint32_t nkeys = 0;
  for (auto it = bucket_view; it != bucket_view + 256; ++it) {
    const int i = it - bucket_view;
    if (it->first) { // there is a key here
      if (it->second % capacity_ == b) {  // in this bucket
        bucket_keys[++nkeys] = it->second;  // copy hash
        it->first = false;  // erase it
        cost[i] = 0;  // slot taken by this bucket, free insertion
      } else {
        cost[i] = UINT16_MAX;  // foreign key, forbid to move
      }
    } else {
      cost[i] = 0;  // empty slot
    }
  }
  // A more interesting cost is (ph_ + 1)^2
  power_index_h128 bucket_index;
  bool ok = bucket_index.Reset(bucket_keys, bucket_keys + nkeys, cost, cost + 256);
  if (ok) {
    CXXMPH_DEBUGLN("Found a ph for %v keys after %v tentatives")(
        nkeys, bucket_index.perfect_hash());
    if (result_ph) *result_ph = bucket_index.perfect_hash();
    return true;
  }
  CXXMPH_DEBUGLN("Failed to find a power index for %v keys")(nkeys);
  return false;
}

POWER_MAP_TMPL_SPEC
bool operator==(const POWER_MAP_CLASS_SPEC& lhs, const POWER_MAP_CLASS_SPEC& rhs) {
  return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

POWER_MAP_TMPL_SPEC POWER_MAP_CLASS_SPEC::power_map() { clear(); }

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
      (it.it_-values_.begin())-(hasher128_.hash128(x.first, seed_)[0] % capacity_));
  assert(it != end());
  return make_pair(it, true);
}

// A fast insert is the tentative of storing a key in its exact location without
// updating power indices.
POWER_MAP_METHOD_DECL(bool_type, fast_insert)(const value_type& x) {
  CXXMPH_DEBUGLN("Trying fast insert for key %v")(x.first);
  auto idx = index(x.first);
  if (!present_[idx]) {
    present_[idx] = true;
    values_[idx] = x;
    ++size_;
    CXXMPH_DEBUGLN("Fast insert for key %v at (%v+%v)")(
        x.first.key, bucket(x.first), idx-bucket(x.first));
    return true;
  }
  CXXMPH_DEBUGLN("Index %v taken by key %v (%v+%v)")(
      idx, values_[idx], bucket(values_[idx].first),
      idx - bucket(values_[idx].first));
  return false;
}

POWER_MAP_METHOD_DECL(bool_type, calculate_bucket_ph)(
    uint32_t b, const h128* new_key, uint8_t* ph) const {
  // Get all keys in the 256 positions covered by this bucket.
  pair<bool, h128> bucket_view[256];
  bool new_key_provisioned = !new_key;
  for (int i = 0; i < 256; ++i) {
    auto idx = b + i;
    auto hk = hasher128_.hash128(values_[idx].first, seed_);
    bucket_view[i] = make_pair(present_[idx], hk);
    CXXMPH_DEBUGLN("Added %v [%v:%v] at (%v+%v) to candidates")(
        values_[idx].first, hk[0], hk[3], b, idx - b);
    if (!present_[idx] && new_key) {  // insert at first empty position
      bucket_view[i] = make_pair(true, *new_key);
      new_key_provisioned = true;
      CXXMPH_DEBUGLN("Added new [%v:%v] at (%v+?) to candidates")(
          (*new_key)[0], (*new_key)[3], b);
    }
  }
  if (!new_key_provisioned) return false;
  uint8_t new_ph;
  bool ok = make_power_index(b, bucket_view, &new_ph);
  if (ok) *ph = new_ph;
  return ok;
}

POWER_MAP_METHOD_DECL(bool_type, slow_insert)(const value_type& x) {
  CXXMPH_DEBUGLN("Trying slow insert")();
  // Find the ph with smallest cost for this insertion
  auto b = bucket(x.first);
  uint8_t ph;
  auto h = hasher128_.hash128(x.first, seed_);
  if (!calculate_bucket_ph(b, &h, &ph)) return false;

  // TODO(davi) Move everyone in the bucket according to the new ph
  for (int i = b; i < b + 256; ++i) {}
    auto idx = b + i;
  }

  // And finally inserts
  auto idx = b + power_index(h, ph);
  CXXMPH_DEBUGLN("Final idx for key %v is (%v+%u)")(
       x.first, b , power_index(h, ph));
  assert((!present_[idx]) || values_[idx] == x);
  values_[idx] = x;
  present_[idx] = true;
  cost_[idx] = keys.size() * keys.size();
  ph_[b] = ph;
  ++size_;
  return true;
}

POWER_MAP_METHOD_DECL(void_type, pack)() {
  int8_t iterations = 16;
  self_type rhs;
  while (iterations--) {
    CXXMPH_DEBUGLN("Trying to PACK - %d iterations left ")(
        iterations);
    if (rhs.Reset(begin(), end())) break;
  }
  assert(iterations > 0);
  this->swap(rhs);
}
POWER_MAP_METHOD_DECL(void_type, swap)(self_type& rhs) {
  CXXMPH_DEBUGLN("Doing swap");
  std::swap(size_, rhs.size_);
  std::swap(capacity_, rhs.capacity_);
  std::swap(seed_, rhs.seed_);
  values_.swap(rhs.values_);
  present_.swap(rhs.present_);
  ph_.swap(rhs.ph_);
}

POWER_MAP_METHOD_DECL(iterator, begin)() { return make_hollow(&values_, &present_, values_.begin()); }
POWER_MAP_METHOD_DECL(iterator, end)() { return make_solid(&values_, &present_, values_.end()); }
POWER_MAP_METHOD_DECL(const_iterator, begin)() const { return make_hollow(&values_, &present_, values_.begin()); }
POWER_MAP_METHOD_DECL(const_iterator, end)() const { return make_solid(&values_, &present_, values_.end()); }
POWER_MAP_METHOD_DECL(bool_type, empty)() const { return size_ == 0; }
POWER_MAP_METHOD_DECL(size_type, size)() const { return size_; }

POWER_MAP_METHOD_DECL(void_type, clear)() {
  Reset(end(), end());
}
POWER_MAP_METHOD_DECL(bool_type, Reset)(
    const_iterator a, const_iterator b) {
  uint32_t nkeys = b - a;
  uint32_t nbuckets = 8;
  while (nbuckets < nkeys) {
    nbuckets = nextpoweroftwo(nbuckets + 1);
  }
  uint32_t siz = 256 + nbuckets;
  decltype(values_)(siz).swap(values_);
  decltype(present_)(siz).swap(present_);
  decltype(ph_)(siz).swap(ph_);
  capacity_ = nbuckets;
  size_ = 0;
  seed_ = random();
  CXXMPH_DEBUGLN("Seed initialized to %v for %v buckets (siz %v)")(
      seed_, nbuckets, siz);
  for (auto it = a; it != b; ++it) {
    if (fast_insert(*it)) continue;
    if (slow_insert(*it)) continue;
    return false;
  }
  return true;
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
  auto b = h[0] % (capacity_);  // TODO(davi) no need for mod
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

}  // namespace cxxmph

#endif  // __CXXMPH_POWER_MAP_H__
