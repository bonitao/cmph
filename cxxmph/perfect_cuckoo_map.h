#ifndef __CXXMPH_PERFECT_CUCKOO_MAP_H__
#define __CXXMPH_PERFECT_CUCKOO_MAP_H__

// Collision math: http://www.wolframalpha.com/input/?i=%28a+*+k+*+e%5E%28-a%29%29%2Fk%21
// From: http://burtleburtle.net/bob/hash/birthday.html

#include <algorithm>
#include <bitset>
#include <unordered_map>  // for std::hash
#include <iostream>
#include <vector>

#include "debruijn_index.h"
#include "seeded_hash.h"
#include "hollow_iterator.h"

namespace cxxmph {

using std::cerr;
using std::endl;
using std::swap;
using std::pair;
using std::make_pair;
using std::vector;

// Save on repetitive typing.
#define PC_MAP_TMPL_SPEC template <class Key, class Data, class HashFcn, class EqualKey, class Alloc>
#define PC_MAP_CLASS_SPEC perfect_cuckoo_map<Key, Data, HashFcn, EqualKey, Alloc>
#define PC_MAP_METHOD_DECL(r, m) PC_MAP_TMPL_SPEC typename PC_MAP_CLASS_SPEC::r PC_MAP_CLASS_SPEC::m
#define PC_MAP_INLINE_METHOD_DECL(r, m) PC_MAP_TMPL_SPEC inline typename PC_MAP_CLASS_SPEC::r PC_MAP_CLASS_SPEC::m

template <class Key, class Data, class HashFcn = std::hash<Key>, class EqualKey = std::equal_to<Key>, class Alloc = std::allocator<Data> >
class perfect_cuckoo_map {
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

  perfect_cuckoo_map();

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;  
  size_type size() const { return n_; }
  bool empty() const;
  void clear();
  void erase(iterator pos);
  void erase(const key_type& k);
  pair<iterator, bool> insert(const value_type& x);
  iterator find(const key_type& k);
  inline const_iterator find(const key_type& k) const;
  data_type& operator[](const key_type &k);

  size_type bucket_count() const { return values_.size(); }
  void rehash(size_type nbuckets /*ignored*/) { pack(true); }

 protected:  // mimicking STL implementation
  EqualKey equal_;

 private:
  bool pack(bool minimal = false);
  bool update_bucket( 
    vector<value_type>* values,
    vector<bool>* present,
    vector<debruijn_index>* index,
    uint32_t b);

  iterator make_iterator(typename std::vector<value_type>::iterator it) {
    return hollow_iterator<std::vector<value_type>>(&values_, &present_, it);
  }
  const_iterator make_iterator(typename std::vector<value_type>::const_iterator it) const {
    return hollow_const_iterator<std::vector<value_type>>(&values_, &present_, it);
  }

  uint32_t n_;  // number of keys
  uint32_t seed_;
  typename seeded_hash<hasher>::hash_function hasher_;

  vector<bool> present_;  // not necessary, but let us reuse iterator work
  typedef vector<debruijn_index> index_type; 
  index_type index_; 
  vector<value_type> values_;
};


PC_MAP_INLINE_METHOD_DECL(iterator, begin)() { return make_hollow(&values_, &present_, values_.begin()); }
PC_MAP_INLINE_METHOD_DECL(iterator, end)() { return make_solid(&values_, &present_, values_.end()); }
PC_MAP_INLINE_METHOD_DECL(const_iterator, begin)() const { return make_hollow(&values_, &present_, values_.begin()); }
PC_MAP_INLINE_METHOD_DECL(const_iterator, end)() const { return make_solid(&values_, &present_, values_.end()); }

PC_MAP_TMPL_SPEC PC_MAP_CLASS_SPEC::perfect_cuckoo_map()
    : n_(0), seed_(random()), index_(2), values_(2*8), present_(2*8) {
}

PC_MAP_METHOD_DECL(insert_return_type, insert)(const value_type& x) {
  auto it = find(x.first);
  auto it_end = end();
  if (it != it_end) return make_pair(it, false);

  uint8_t iterations = 4;
  while (iterations--) {
    auto h = hasher_(x.first, seed_);
    auto b = h & (index_.size() - 2);
    bool success = false;
    auto pbegin = present_.begin() + b*8;
    for (auto pit = pbegin; pit != pbegin + 16; ++pit) {
      // for debruijn index only
      if (pit >= pbegin + 8 && pit < pbegin + 12) continue;
      if (!*pit) {
        values_[pit - present_.begin()] = x;
        // cerr << "Initially inserted key " << x.first << " at position "
        //     << (pit - present_.begin()) << endl;
        *pit = true;
        success = true;
        break;
      }
    }
    if (success) break;
    if (!pack()) exit(-1);
  }
  if (iterations < 0) {
    fprintf(stderr, "Pack failed\n");
    exit(-1);
  }
  ++n_;
  auto h = hasher_(x.first, seed_);
  auto b = h & (index_.size() - 2);
  // cerr << "Final bucket: " << b << endl;
  if (!update_bucket(&values_, &present_, &index_, b)) {
    fprintf(stderr, "update bucket failed\n");
    exit(-1);
  }
  it = find(x.first);
  assert(it != end());
  return make_pair(it, true);
}

PC_MAP_METHOD_DECL(bool_type, pack)(bool minimal) {
  // cerr << "Running pack with " << n_ << " keys " << endl;
  vector<value_type> new_values((values_.size() - 8)*2 + 8);
  vector<bool> new_present(new_values.size(), false);
  // cerr << "values_.size " << values_.size() << " new_values.size " << new_values.size() << endl;
  vector<debruijn_index> new_index(new_values.size()/8);
  vector<vector<value_type>> buckets;
  int iterations = 16;
  while (iterations--) {
    bool success = true;
    vector<vector<value_type>> new_buckets(new_index.size());
    int seed = random();
    assert(size() <= new_buckets.size()*8);
    for (auto it = begin(); it != end(); ++it) {
      auto h = hasher_(it->first, seed);
      auto b = h & (new_index.size() - 2);
      // cerr << "Key " << it->first << " into bucket " << b << " mod " << (new_index.size()-1) << endl;
      new_buckets[b].push_back(*it);
    }
    assert(new_buckets[new_buckets.size()-1].size() == 0);  // pivot
    for (auto it = new_buckets.begin(); it != new_buckets.end() - 1; ++it) {
      if (it->size() > 16) success = false;;
      if (it->size() + (it+1)->size() > 24) success = false;;
      // cerr << "bucket " << it - new_buckets.begin() << " has " << it->size() << " keys " << endl;
    }
    if (success) {
      seed_ = seed;
      buckets.swap(new_buckets);
      for (int i = 0; i < values_.size(); ++i) {
        values_[i] = value_type();
        present_[i] = false;
      }
      break;
    }
  }
  if (iterations < 0) {
    assert(0);
    return false;
  }
  for (int i = 0; i < buckets.size() - 1; ++i) {
    auto pit = new_present.begin() + (i<<3);
    auto vit = new_values.begin() + (i<<3);
    // cerr << "Bucket " << i << " starts at " << (pit - new_present.begin()) << endl;
    for (int j = 0; j < buckets[i].size(); ++j, ++pit, ++vit) {
      *pit = true;
      *vit = buckets[i][j];
      // cerr << "Initially added key " << vit->first << " at pos " << (pit - new_present.begin()) << endl;
    }
    if (!update_bucket(&new_values, &new_present, &new_index, i)) {
      assert(0);
      return false;
    }
  }
  new_values.swap(values_);
  new_present.swap(present_);
  new_index.swap(index_);
  return true;
}

PC_MAP_METHOD_DECL(bool_type, update_bucket)(
    vector<value_type>* values,
    vector<bool>* present,
    vector<debruijn_index>* index,
    uint32_t b) {
  // cerr << "Updating bucket " << b << endl;
  vector<uint32_t> hashes;
  vector<uint32_t> oldpos;
  vector<value_type> tmpvalues;
  int m = 8;
  auto pbegin = present->begin() + b*8;
  for (auto pit = pbegin; pit != pbegin + 16; ++pit) {
    assert(pit != present->end());
    auto pos = pit - present->begin();
    if (!*pit) {
      // cerr << "skipping empty pos " << pos << endl;
      continue;
    }
    auto x = (*values)[pos];
    auto h = hasher_(x.first, seed_);
    auto xb = h & (index->size() - 2);
    if (xb != b) {
      if ((pos - (pbegin - present->begin())) < 8) --m;
      continue;
    }
    hashes.push_back(h);
    tmpvalues.push_back(x);
    oldpos.push_back(pos);
    // cerr << "Storing key " << x.first << " from oldpos " << pos << " at bucket " << b << endl;
  }
  // cerr << "Ready to recreate bucket index for " << tmpvalues.size() << " keys  with m "  << m << endl;
  if (!((*index)[b]).reset(hashes, m)) return false;
  // cerr << "Ready to rearrange bucket values" << endl;
  for (int i = 0; i < oldpos.size(); ++i) {
    // cerr << "Cleaning oldpos " << oldpos[i] << endl;
    (*values)[oldpos[i]] = value_type();
    (*present)[oldpos[i]] = false;
  }
  for (int i = 0; i < hashes.size(); ++i) {
    uint32_t pos = (*index)[b].index(hashes[i]) + (b << 3);
    (*values)[pos] = tmpvalues[i];
    // cerr << "Inserting key " << (*values)[pos].first << " at pos " << pos << endl;
    assert(!((*present)[pos]));
    (*present)[pos] = true;
  }
  return true;
}

PC_MAP_METHOD_DECL(iterator, find)(const key_type& k) {
  auto h = hasher_(k, seed_);
  auto b = h & (index_.size() - 2);
  auto pos = (b<<3) + index_[b].index(h);
  // cerr << "Looking for key " << k << " at bucket " << b << " pos " << pos << endl;
  if (__builtin_expect(present_[pos], true)) { 
    auto vit = values_.begin() + pos;
    if (equal_(k, vit->first)) return make_iterator(vit);
  }
  return end();
}

PC_MAP_INLINE_METHOD_DECL(const_iterator, find)(const key_type& k) const {
  auto h = hasher_(k, seed_);
  auto b = h & (index_.size() - 2);
  __builtin_prefetch(&(*(values_.begin() + (b<<3))));
  auto pos = (b<<3) + index_[b].index(h);
  // cerr << "Looking for key " << k << " at bucket " << b << " pos " << pos << endl;
  // if (__builtin_expect(present_[pos], true)) { 
  auto vit = values_.begin() + pos;
  return make_iterator(vit);
  // if (__builtin_expect(equal_(k, vit->first), true)) return make_iterator(vit);
  return end();
}

PC_MAP_METHOD_DECL(data_type&, operator[])(const key_type& k) {
  return insert(make_pair(k, data_type())).first->second;
}

PC_MAP_METHOD_DECL(void_type, erase)(iterator pos) {
  int i = 0;
  auto it = pos.fill_iterator();
  assert(present_[it - values_.begin()]);
  present_[it - values_.begin()] = false;
  *pos = value_type();
  --n_;
}

PC_MAP_METHOD_DECL(void_type, erase)(const key_type& k) {
  iterator it = find(k);
  if (it == end()) {
    // cerr << "Not erasing unfound key " << k << endl;
    return;
  }
  erase(it);
}




}  // namespace cxxmph
#endif
