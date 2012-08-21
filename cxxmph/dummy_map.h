#ifndef __CXXMPH_DUMMY_MAP_H__
#define __CXXMPH_DUMMY_MAP_H__

// Associative mapping implementation assuming no collisions.
//
// This class demands that the key implements the concepts of struct
// bucketed_key. The easiest way to do it is to pass your class as a template
// parameter to it.

#include <algorithm>
#include <bitset>
#include <unordered_map>  // for std::hash
#include <utility>
#include <iostream>
#include <vector>

#include "string_util.h"
#include "seeded_hash.h"

namespace cxxmph {

using std::cerr;
using std::endl;
using std::swap;
using std::pair;
using std::make_pair;
using std::vector;

// Save on repetitive typing.
#define DUMMY_MAP_TMPL_SPEC template <class Key, class Data, class HashFcn, class EqualKey, class Alloc>
#define DUMMY_MAP_CLASS_SPEC dummy_map<Key, Data, HashFcn, EqualKey, Alloc>
#define DUMMY_MAP_METHOD_DECL(r, m) DUMMY_MAP_TMPL_SPEC typename DUMMY_MAP_CLASS_SPEC::r DUMMY_MAP_CLASS_SPEC::m
#define DUMMY_MAP_INLINE_METHOD_DECL(r, m) DUMMY_MAP_TMPL_SPEC inline typename DUMMY_MAP_CLASS_SPEC::r DUMMY_MAP_CLASS_SPEC::m

template <class Key, class Data, class HashFcn = std::hash<Key>, class EqualKey = std::equal_to<Key>, class Alloc = std::allocator<Data> >
class dummy_map {
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

  typedef typename vector<value_type>::iterator iterator;
  typedef typename vector<value_type>::const_iterator const_iterator;

  typedef void void_type;
  typedef bool bool_type;
  typedef pair<iterator, bool> insert_return_type;

  dummy_map();
  ~dummy_map();

  iterator begin() { return values_.begin(); }
  iterator end() { return values_.end(); }
  const_iterator begin() const { return values_.begin(); }
  const_iterator end() const { return values_.end(); }
  size_type size() const { return n_; }
  bool empty() const { return n_ == 0; }
  void clear() { values_.clear(); }
  void erase(iterator pos);
  void erase(const key_type& k);
  pair<iterator, bool> insert(const value_type& x);
  iterator find(const key_type& k);
  inline const_iterator find(const key_type& k) const;
  data_type& operator[](const key_type &k);
  typedef int32_t my_int32_t;  // help macros
  inline int32_t index(const key_type& k) const;

  size_type bucket_count() const { return values_.size(); }
  void rehash(size_type nbuckets /*ignored*/) { }

 protected:  // mimicking STL implementation
  EqualKey equal_;

 private:
  uint32_t n_;  // number of keys
  hasher hasher_;  // unused
  vector<value_type> values_;
  vector<bool> present_;
};

DUMMY_MAP_TMPL_SPEC DUMMY_MAP_CLASS_SPEC::dummy_map()
    : n_(0) { }

DUMMY_MAP_TMPL_SPEC DUMMY_MAP_CLASS_SPEC::~dummy_map() {}

DUMMY_MAP_METHOD_DECL(insert_return_type, insert)(const value_type& x) {
  auto it = find(x.first);
  auto it_end = end();
  if (it != it_end) { return make_pair(it, false); }

  auto insert_index = x.first.bucket;
  if (insert_index >= values_.size()) {
    present_.resize(insert_index+1);
    values_.resize(insert_index+1);
  }
  values_[insert_index] = x;
  present_[insert_index] = true;
  ++n_;
  return make_pair(values_.begin() + insert_index, true);
}

DUMMY_MAP_METHOD_DECL(iterator, find)(const key_type& k) {
  if (k.bucket < values_.size() && present_[k.bucket]) {
    auto vit = values_.begin() + k.bucket;
    if (equal_(k, vit->first)) return vit;
  }
  return end();
}

DUMMY_MAP_INLINE_METHOD_DECL(const_iterator, find)(const key_type& k) const {
  if (k.bucket < values_.size() && present_[k.bucket]) {
    auto vit = values_.begin() + k.second;
    if (equal_(k, vit->first)) return vit;
  }
  return end();
}

DUMMY_MAP_METHOD_DECL(data_type&, operator[])(const key_type& k) {
  return insert(make_pair(k, data_type())).first->second;
}

DUMMY_MAP_METHOD_DECL(void_type, erase)(iterator pos) {
  *pos = value_type(); --n_;
}

DUMMY_MAP_METHOD_DECL(void_type, erase)(const key_type& k) {
  iterator it = find(k);
  if (it == end()) return;
  erase(it);
}

}  // namespace cxxmph
#endif
