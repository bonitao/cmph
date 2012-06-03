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
#include <unordered_map>
#include <vector>
#include <utility>  // for std::pair

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
  inline int32_t index(const key_type& k) const;
  data_type& operator[](const key_type &k);
  const data_type& operator[](const key_type &k) const;

  size_type bucket_count() const { return ph_.size(); }
  void rehash(size_type nbuckets); 

 protected:  // mimicking STL implementation
  EqualKey equal_;

 private:
  void pack();
  bool fast_insert(const value_type& x);
  bool slow_insert(const value_type& x);
  bool calculate_bucket_ph(
    uint32_t pos, const h128* new_key,
    uint8_t* ph, vector<h128>* keys);

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
bool make_power_index(
    const vector<h128>& keys, const vector<uint32_t>& cost, uint8_t* ph);


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
  cerr << "Inserting key " << x.first << endl;
  auto it = find(x.first);
  auto it_end = end();
  if (it != it_end) return make_pair(it, false);
  while (!(fast_insert(x) || slow_insert(x))) pack();
  it = find(x.first);
  cerr << "Inserted key " << x.first << " (pos " << (hasher128_.hash128(x.first, seed_)[0] % capacity_) << ") at bucket " << (it.it_-values_.begin()) << endl;
  assert(it != end());
  return make_pair(it, true);
}

POWER_MAP_METHOD_DECL(bool_type, fast_insert)(const value_type& x) {
  cerr << "Trying fast insert " << endl;
  auto idx = index(x.first);
  if (ph_[idx] == 0 && !present_[idx]) {
    ph_[idx] = 1;
    present_[idx] = true;
    values_[idx] = x;
    return true;
  }
  return false;
}

POWER_MAP_METHOD_DECL(bool_type, calculate_bucket_ph)(
    uint32_t pos, const h128* new_key, uint8_t* ph, vector<h128>* keys) {
  // Get keys and cost
  uint8_t nkeys = 0;
  h128 keys_array[256];
  vector<uint32_t> cost(cost_.begin() + pos,
                        cost_.begin() + pos + 256);;
  for (int i = 0; i < 256; ++i) {
    if (!present_[pos + i]) continue;
    auto h = hasher128_.hash128(values_[pos + i].first, seed_);
    if (h[0] % capacity_ == pos) {
      cost[i] = 0;
      keys_array[nkeys++] = h;
    }
  }
  if (new_key) keys_array[nkeys++] = *new_key;
  vector<h128>(keys_array, keys_array + nkeys).swap(*keys);
  cerr << "Making power index for " << keys->size() << " keys at bucket " << pos  << endl;
  bool r = make_power_index(*keys, cost, ph);
  cerr << "New key has power index " << static_cast<uint16_t>(power_index(*new_key, *ph)) << endl;
  return r;
}

POWER_MAP_METHOD_DECL(bool_type, slow_insert)(const value_type& x) {
  cerr << "Trying slow insert " << endl;
  // Find the ph with smallest cost for this insertion
  auto h = hasher128_.hash128(x.first, seed_);
  auto pos = h[0] % capacity_;
  uint8_t ph;
  vector<h128> keys;
  if (!calculate_bucket_ph(pos, &h, &ph, &keys)) return false; 

  // Update cost vector and rollback for cost vector if insertion fails
  unordered_map<uint32_t, uint32_t> cost_rollback;
  for (auto hit = keys.begin(); hit != keys.end(); ++hit) {
    auto idx = pos + power_index(*hit, ph);
    cost_rollback[idx] = cost_[idx];
    cerr << "Updating bucket " << idx << " cost to infinite " << endl;
    cost_[idx] = 1ULL << 16;
  }

  // Remove and reinsert under new cost vector any key using a bucket needed by
  // the generated ph
  for (auto hit = keys.begin(); hit != keys.end(); ++hit) {
    auto idx = pos + power_index(*hit, ph);
    auto hf = hasher128_.hash128(values_[idx].first, seed_);
    if (present_[idx] && hf[0] % capacity_ != pos) {
      auto y = values_[idx];
      cerr << "Pushing key " << y.first << " at bucket " << idx << endl;
      values_[idx] = x;
      bool pushed = slow_insert(y);
      if (!pushed) break;
      values_[idx] = y;
      for (auto cit = cost_rollback.begin(); cit != cost_rollback.end(); ++cit) {
        assert(cost_[cit->first] == 1ULL<<16);
        cost_[cit->first] = cit->second;
      }
      values_[idx] = value_type();
    }
  }

  // TODO(davi) correctly update cost vector

  // Redistributes the keys for this new ph
  for (int i = 0; i < 256; ++i) {
    if (!present_[pos + i]) continue;
    auto hf = hasher128_.hash128(values_[pos + i].first, seed_);
    if (hf[0] % capacity_ != pos) continue;
    auto idx = pos + power_index(hf, ph);
    if (idx == pos + i) continue;
    assert(pos + i == pos + power_index(hf, ph_[pos]));
    cerr << "Swapping keys at " << (pos + i) << " and " << idx << endl;
    swap(values_[pos + i], values_[idx]);
    bool tmp = present_[pos + i];
    present_[pos + i] = present_[idx];
    present_[idx] = tmp;
    assert(present_[idx]);

    // pos + i either belongs to this bucket or is empty
    cost_[idx] = keys.size() * keys.size();
    if (!present_[pos + i]) cost_[pos + i] = 0;
    else cost_[pos + i] = keys.size() * keys.size();

    --i;
  }

  // And finally inserts
  auto idx = pos + power_index(h, ph);
  cerr << "Final idx for key " << x.first << " is (" << pos << "+" << static_cast<uint16_t>(power_index(h, ph)) << ")" << endl;
  values_[idx] = x;
  present_[idx] = true;
  ph_[pos] = ph;
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
  cerr << "Find at idx " << idx << " present " << static_cast<uint16_t>(present_[idx])
       << " equals " << static_cast<uint16_t>(vit->first == k) << endl;
  if (!present_[idx] || vit->first != k) return end();
  return make_solid(&values_, &present_, vit);;
}

POWER_MAP_INLINE_METHOD_DECL(my_int32_t, index)(const key_type& k) const {
  auto h = hasher128_.hash128(k, seed_);
  auto pos = h[0] % (capacity_);
  auto ph = ph_[pos];
  auto idx = pos + power_index(h, ph);
  cerr << "key " << k << " seed " << seed_ << " pos " << pos << " ph " << static_cast<uint16_t>(ph) << " idx " << idx << endl; 
  return idx;
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
