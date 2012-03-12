#ifndef __CXXMPH_MPH_MAP_H__
#define __CXXMPH_MPH_MAP_H__
// Implementation of the unordered associative mapping interface using a
// minimal perfect hash function.
//
// This class is about 20% to 100% slower than unordered_map (or ext/hash_map)
// and should not be used if performance is a concern. In fact, you should only
// use it for educational purposes.

#include <algorithm>
#include <unordered_map>
#include <vector>
#include <utility>  // for std::pair

#include "MurmurHash2.h"
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

  size_type bucket_count() const { return index_.perfect_hash_size() + slack_.bucket_count(); }
  // FIXME: not sure if this has the semantics I want
  void rehash(size_type nbuckets /*ignored*/) { pack(); }

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
   // TODO(davi) optimize slack to no hold a copy of the key
   typedef unordered_map<Key, uint32_t, HashFcn, EqualKey, Alloc> slack_type;
   slack_type slack_;
   size_type size_;
};

MPH_MAP_TMPL_SPEC
bool operator==(const MPH_MAP_CLASS_SPEC& lhs, const MPH_MAP_CLASS_SPEC& rhs) {
  return lhs.size() == rhs.size() && std::equal(lhs.begin(), lhs.end(), rhs.begin());
}

MPH_MAP_TMPL_SPEC MPH_MAP_CLASS_SPEC::mph_map() : size_(0) {
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
  slack_.insert(make_pair(x.first, values_.size() - 1));
  if (should_pack) pack();
  it = find(x.first);
  return make_pair(it, true);
}

MPH_MAP_METHOD_DECL(void_type, pack)() {
  if (values_.empty()) return;
  bool success = index_.Reset(
      make_iterator_first(begin()),
      make_iterator_first(end()), size_);
  assert(success);
  std::vector<value_type> new_values(index_.size());
  std::vector<bool> new_present(index_.size(), false);
  for (iterator it(begin()), it_end(end()); it != it_end; ++it) {
    size_type id = index_.index(it->first);
    assert(id < new_values.size());
    new_values[id] = *it;
    new_present[id] = true;
  }
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
}

MPH_MAP_METHOD_DECL(void_type, erase)(iterator pos) {
  present_[pos - begin] = false;
  *pos = value_type();
}
MPH_MAP_METHOD_DECL(void_type, erase)(const key_type& k) {
  iterator it = find(k);
  if (it == end()) return;
  erase(it);
}

MPH_MAP_METHOD_DECL(const_iterator, find)(const key_type& k) const {
  if (__builtin_expect(!slack_.empty(), 0)) {
     auto it = slack_.find(k);
     if (it != slack_.end()) return make_iterator(values_.begin() + it->second);
  }
  if (__builtin_expect(index_.size() == 0, 0)) return end();
  auto id = index_.perfect_hash(k);
  if (!present_[id]) return end();
  auto it = make_iterator(values_.begin() + id);
  if (__builtin_expect(equal_(k, it->first), 1)) return it;
  return end();
}

MPH_MAP_METHOD_DECL(iterator, find)(const key_type& k) {
  if (__builtin_expect(!slack_.empty(), 0)) {
     auto it = slack_.find(k);
     if (it != slack_.end()) return make_iterator(values_.begin() + it->second);
  }
  if (__builtin_expect(index_.size() == 0, 0)) return end();
  auto id = index_.perfect_hash(k);
  if (!present_[id]) return end();
  auto it = make_iterator(values_.begin() + id);
  if (__builtin_expect(equal_(k, it->first), 1)) return it;
  return end();
}

MPH_MAP_METHOD_DECL(my_int32_t, index)(const key_type& k) const {
  if (index_.size() == 0) return -1;
  return index_.perfect_hash(k);
}

MPH_MAP_METHOD_DECL(data_type&, operator[])(const key_type& k) {
  return insert(make_pair(k, data_type())).first->second;
}

}  // namespace cxxmph

#endif  // __CXXMPH_MPH_MAP_H__
