#ifndef __CXXMPH_MPH_MAP_H__
#define __CXXMPH_MPH_MAP_H__
// Implementation of the unordered associative mapping interface using a
// minimal perfect hash function.
//
// This class not necessarily faster than unordered_map (or ext/hash_map).
// Benchmark your code before using it. If you do not call rehash() before
// starting your reads, it will be definitively slower than unordered_map.
//
// For large sets of urls, which are a somewhat expensive to compare, I found
// this class to be about 10% faster than unordered_map.
//
// The space overhead of this map is 1.93 bits per bucket and it achieves 100%
// occupation with a rehash call.

#include <algorithm>
#include <iostream>
#include <limits>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <utility>  // for std::pair

#include "mph_bits.h"
#include "mph_index.h"
#include "hollow_iterator.h"

namespace cxxmph {

using std::pair;
using std::make_pair;
using std::unordered_map;
using std::vector;

// Save on repetitive typing.
#define MPH_MAP_TMPL_SPEC template <class Key, class Data, class HashFcn, class EqualKey, class Alloc>
#define MPH_MAP_CLASS_SPEC mph_map<Key, Data, HashFcn, EqualKey, Alloc>
#define MPH_MAP_METHOD_DECL(r, m) MPH_MAP_TMPL_SPEC typename MPH_MAP_CLASS_SPEC::r MPH_MAP_CLASS_SPEC::m

template <class Key, class Data, class HashFcn = std::hash<Key>, class EqualKey = std::equal_to<Key>, class Alloc = std::allocator<Data> >
class mph_map {
 public:
  typedef Key key_type;
  typedef Data data_type;
  typedef pair<Key, Data> value_type;
  typedef HashFcn hasher;
  typedef EqualKey key_equal;

  typedef typename std::vector<value_type>::pointer pointer;
  typedef typename std::vector<value_type>::reference reference;
  typedef typename std::vector<value_type>::const_reference const_reference;
  typedef typename std::vector<value_type>::size_type size_type;
  typedef typename std::vector<value_type>::difference_type difference_type;

  typedef hollow_iterator<std::vector<value_type>> iterator;
  typedef hollow_const_iterator<std::vector<value_type>> const_iterator;

  // For making macros simpler.
  typedef void void_type;
  typedef bool bool_type;
  typedef pair<iterator, bool> insert_return_type;

  mph_map();
  ~mph_map();

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
  iterator find(const key_type& k);
  const_iterator find(const key_type& k) const;
  typedef int32_t my_int32_t;  // help macros
  int32_t index(const key_type& k) const;
  data_type& operator[](const key_type &k);
  const data_type& operator[](const key_type &k) const;

  size_type bucket_count() const { return index_.minimal_perfect_hash_size() + slack_.bucket_count(); }
  void rehash(size_type nbuckets /*ignored*/); 

 protected:  // mimicking STL implementation
  EqualKey equal_;

 private:
   template <typename iterator>
   struct iterator_first : public iterator {
     iterator_first(iterator it) : iterator(it) { }
     const typename iterator::value_type::first_type& operator*() {
      return this->iterator::operator*().first;
     }
   };

   template <typename iterator>
     iterator_first<iterator> make_iterator_first(iterator it) {
     return iterator_first<iterator>(it);
   }

   iterator make_iterator(typename std::vector<value_type>::iterator it) {
     return hollow_iterator<std::vector<value_type>>(&values_, &present_, it);
   }
   const_iterator make_iterator(typename std::vector<value_type>::const_iterator it) const {
     return hollow_const_iterator<std::vector<value_type>>(&values_, &present_, it);
   }

   void pack();
   std::vector<value_type> values_;
   std::vector<bool> present_;
   SimpleMPHIndex<Key, typename seeded_hash<HashFcn>::hash_function> index_;
   // TODO(davi) optimize slack to use hash from index rather than calculate its own
   typedef unordered_map<h128, uint32_t, h128::hash32> slack_type;
   slack_type slack_;
   size_type size_;
   typename seeded_hash<HashFcn>::hash_function hasher128_;
};

MPH_MAP_TMPL_SPEC
bool operator==(const MPH_MAP_CLASS_SPEC& lhs, const MPH_MAP_CLASS_SPEC& rhs) {
  return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

MPH_MAP_TMPL_SPEC MPH_MAP_CLASS_SPEC::mph_map() : size_(0) {
  clear();
  pack();
}

MPH_MAP_TMPL_SPEC MPH_MAP_CLASS_SPEC::~mph_map() {
}

MPH_MAP_METHOD_DECL(insert_return_type, insert)(const value_type& x) {
  auto it = find(x.first);
  auto it_end = end();
  if (it != it_end) return make_pair(it, false);
  bool should_pack = false;
  if (values_.capacity() == values_.size() && values_.size() > 256) {
    should_pack = true;
  }
  values_.push_back(x);
  present_.push_back(true);
  ++size_;
  h128 h = hasher128_.hash128(x.first, 0);
  if (slack_.find(h) != slack_.end()) should_pack = true;  // unavoidable pack
  else slack_.insert(std::make_pair(h, values_.size() - 1));
  if (should_pack) pack();
  it = find(x.first);
  return make_pair(it, true);
}

MPH_MAP_METHOD_DECL(void_type, pack)() {
  // fprintf(stderr, "Paki %d values\n", values_.size());
  if (values_.empty()) return;
  assert(std::unordered_set<key_type>(make_iterator_first(begin()), make_iterator_first(end())).size() == size());
  bool success = index_.Reset(
      make_iterator_first(begin()),
      make_iterator_first(end()), size_);
  assert(success);
  std::vector<value_type> new_values(index_.minimal_perfect_hash_size());
  new_values.reserve(new_values.size() * 2);
  std::vector<bool> new_present(index_.minimal_perfect_hash_size(), false);
  new_present.reserve(new_present.size() * 2);
  for (iterator it = begin(), it_end = end(); it != it_end; ++it) {
    size_type id = index_.minimal_perfect_hash(it->first);
    assert(id < index_.minimal_perfect_hash_size());
    assert(id < new_values.size());
    new_values[id] = *it;
    new_present[id] = true;
  }
  // fprintf(stderr, "Collision ratio: %f\n", collisions*1.0/size());
  values_.swap(new_values);
  present_.swap(new_present);
  slack_type().swap(slack_);
}

MPH_MAP_METHOD_DECL(iterator, begin)() { return make_iterator(values_.begin()); }
MPH_MAP_METHOD_DECL(iterator, end)() { return make_iterator(values_.end()); }
MPH_MAP_METHOD_DECL(const_iterator, begin)() const { return make_iterator(values_.begin()); }
MPH_MAP_METHOD_DECL(const_iterator, end)() const { return make_iterator(values_.end()); }
MPH_MAP_METHOD_DECL(bool_type, empty)() const { return size_ == 0; }
MPH_MAP_METHOD_DECL(size_type, size)() const { return size_; }

MPH_MAP_METHOD_DECL(void_type, clear)() {
  values_.clear();
  present_.clear();
  slack_.clear();
  index_.clear();
  size_ = 0;
}

MPH_MAP_METHOD_DECL(void_type, erase)(iterator pos) {
  present_[pos - begin] = false;
  *pos = value_type();
  --size_;
}
MPH_MAP_METHOD_DECL(void_type, erase)(const key_type& k) {
  iterator it = find(k);
  if (it == end()) return;
  erase(it);
}

MPH_MAP_METHOD_DECL(const_iterator, find)(const key_type& k) const {
  if (__builtin_expect(index_.minimal_perfect_hash_size(), 1)) {
    auto minimal_perfect_hash = index_.minimal_perfect_hash(k);
    if (__builtin_expect(present_[minimal_perfect_hash], true)) { 
      auto vit = values_.begin() + minimal_perfect_hash;
      if (equal_(k, vit->first)) return make_iterator(vit);
    }
  }
  if (__builtin_expect(!slack_.empty(), 0)) {
     auto sit = slack_.find(hasher128_.hash128(k, 0));
     if (sit != slack_.end()) return make_iterator(values_.begin() + sit->second);
  }
  return end();
}

MPH_MAP_METHOD_DECL(iterator, find)(const key_type& k) {
  if (__builtin_expect(index_.minimal_perfect_hash_size(), 1)) {
    auto minimal_perfect_hash = index_.minimal_perfect_hash(k);
    if (__builtin_expect(present_[minimal_perfect_hash], true)) { 
      auto vit = values_.begin() + minimal_perfect_hash;
      if (equal_(k, vit->first)) return make_iterator(vit);
    }
  }
  if (__builtin_expect(!slack_.empty(), 0)) {
     auto sit = slack_.find(hasher128_.hash128(k, 0));
     if (sit != slack_.end()) return make_iterator(values_.begin() + sit->second);
  }
  return end();
}

MPH_MAP_METHOD_DECL(my_int32_t, index)(const key_type& k) const {
  if (index_.size() == 0) return -1;
  return index_.minimal_perfect_hash(k);
}

MPH_MAP_METHOD_DECL(data_type&, operator[])(const key_type& k) {
  return insert(make_pair(k, data_type())).first->second;
}
MPH_MAP_METHOD_DECL(void_type, rehash)(size_type nbuckets) {
  pack();
  vector<value_type>(values_.begin(), values_.end()).swap(values_);
  vector<bool>(present_.begin(), present_.end()).swap(present_);
  slack_type().swap(slack_);
}


}  // namespace cxxmph

#endif  // __CXXMPH_MPH_MAP_H__
