#ifndef __CXXMPH_PERFECT_CUCKOO_H__
#define __CXXMPH_PERFECT_CUCKOO_H__

#include <algorithm>
#include <bitset>
#include <unordered_map>  // for std::hash
#include <iostream>
#include <vector>

#include "perfect_cuckoo_cache_line.h"
#include "seeded_hash.h"
#include "flatten_iterator.h"

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

  typedef vector<vector<value_type> > values_type;
  typedef flatten_iterator<values_type> iterator;
  typedef flatten_const_iterator<values_type> const_iterator;

  // For making macros simpler.
  typedef void void_type;
  typedef bool bool_type;
  typedef pair<iterator, bool> insert_return_type;

  perfect_cuckoo_map();

  iterator begin() { return make_flatten_begin(&values_); }
  iterator end() { return make_flatten_end(&values_); }
  const_iterator begin() const { return make_flatten_begin(&values_); }
  const_iterator end() const { return make_flatten_end(&values_); }
  size_type size() const { return n_; }
  bool empty() const;
  void clear();
  void erase(iterator pos);
  void erase(const key_type& k);
  pair<iterator, bool> insert(const value_type& x);
  iterator find(const key_type& k);
  const_iterator find(const key_type& k) const;
  data_type& operator[](const key_type &k);

  size_type bucket_count() const { return values_.size(); }
  void rehash(size_type nbuckets /*ignored*/) { pack(true); }

 private:
  bool pack(bool minimal = false);
  bool pack_bucket(uint32_t b);
  bool make_pccl(const vector<uint32_t>& keys, perfect_cuckoo_cache_line* pccl) const;
  void make_bucket(const perfect_cuckoo_cache_line& pccl, uint32_t b);

  uint32_t n_;  // number of keys
  uint32_t seed_;
  typename seeded_hash<hasher>::hash_function hasher_;

  values_type values_;
  typedef vector<pair<perfect_cuckoo_cache_line, typename vector<value_type>::iterator>> index_type; 
  index_type index_; 
};

PC_MAP_TMPL_SPEC PC_MAP_CLASS_SPEC::perfect_cuckoo_map()
    : n_(0), seed_(random()), index_(1), values_(1) {
}

PC_MAP_METHOD_DECL(insert_return_type, insert)(const value_type& x) {
  auto it = find(x.first);
  auto it_end = end();
  if (it != it_end) return make_pair(it, false);

  auto h = hasher_(x.first, seed_);
  auto b = h & (index_.size() - 1);
  // cerr << "Inserting key " << x.first << " on bucket " << b << endl;
  assert(values_.size() > b);
  values_[b].push_back(x);
  ++n_;
  if (!index_[b].first.insert(h)) {
    if ((!pack_bucket(b)) || values_[b].size() > perfect_cuckoo_cache_line::good_capacity()) {
      if (!pack()) {
        fprintf(stderr, "Failed to pack\n");
        exit(-1);
      }
    }
  } else make_bucket(index_[b].first, b);
  it = find(x.first);
  assert(it != end());
  return make_pair(it, true);
}

PC_MAP_METHOD_DECL(bool_type, pack)(bool minimal) {
  int iterations = 16;
  uint32_t index_size = index_.size() * 2;
  fprintf(stderr, "Growing index size to %d\n", index_size);
  uint32_t seed = 0;
  while (iterations--) {
    index_type index(index_size);
    vector<vector<uint32_t>> hashes(index_size);
    auto success = true;
    seed = random();
    for (auto it = begin(), i = 0; it != end(); ++it, ++i) {
      auto h = hasher_(it->first, seed);
      auto b = h & (index.size() - 1);
      // fprintf(stderr, "Adding key %d h %llu at bucket %d\n", it->first, h, b);
      hashes[b].push_back(h);
    }
    for (uint32_t i = 0; i < index_size; ++i) {
      // fprintf(stderr, "pccl construction for bucket %d\n", i);
      if (!make_pccl(hashes[i], &index[i].first)) {
        success = false;
        break;
      }
      assert(index[i].first.size() == hashes[i].size());
    }
    if (!success) continue;
    vector<vector<uint32_t>>().swap(hashes);
    values_type values(index_size);
    for (int i = 0; i < index_size; ++i) values[i].resize(index[i].first.size());
    for (auto it = begin(), i = 0; it != end(); ++it, ++i) {
      auto h = hasher_(it->first, seed);
      auto b = h & (index.size() - 1);
      auto mph = index[b].first.minimal_perfect_hash(h);
      assert(mph < index[b].first.size());
      assert(mph < values[b].size());
      values[b][mph] = *it; 
    }
    values.swap(values_);
    index.swap(index_);
    seed_ = seed;
    break;
  }
  return iterations >= 0;
}

PC_MAP_METHOD_DECL(bool_type, make_pccl)(
   const vector<uint32_t>& keys, perfect_cuckoo_cache_line* pccl) const {
  assert(keys.size() < perfect_cuckoo_cache_line::max_capacity());
  int iterations = 16;
  while (iterations--) {
    pccl->set_seed(random());
    pccl->clear();
    bool success = true;
    // fprintf(stderr, "Trying to make pccl with seed %d for %d keys\n", pccl->seed(), keys.size());
    for (auto it = keys.begin(), end = keys.end(); it != end; ++it) {
      if (!pccl->insert(*it)) {
        success = false;
        break;
      }
    }
    if (success) break;
  }
  assert(iterations < 0 || pccl->size() == keys.size());
  // fprintf(stderr, "make pccl iterations unneeded: %d\n", iterations);
  return iterations >= 0;
}

PC_MAP_METHOD_DECL(void_type, make_bucket)(
   const perfect_cuckoo_cache_line& pccl, uint32_t b) {
  vector<value_type>* bucket = &values_[b];
  vector<value_type> v(bucket->size());
  assert(v.size() == pccl.size());
  // FIXME: need to iterate over value_type() first to prevent overrides
  for (auto it = bucket->begin(), end = bucket->end(); it != end; ++it) {
    auto h = hasher_(it->first, seed_);
    auto mph = pccl.minimal_perfect_hash(h);
    // fprintf(stderr, "Moving key h %llu mph %d\n", h, mph);
    assert(mph < pccl.size());
    v[mph] = *it;
  }
  swap(*bucket, v);
  index_[b].second = bucket->begin();
}

PC_MAP_METHOD_DECL(bool_type, pack_bucket)(uint32_t b) {
  // fprintf(stderr, "Packing bucket %d with %d keys\n", b, values_[b].size());
  vector<uint32_t> keys(values_[b].size());
  for (int i = 0; i < values_[b].size(); ++i) {
    keys[i] = hasher_(values_[b][i].first, seed_);
  }
  perfect_cuckoo_cache_line pccl;
  if (!make_pccl(keys, &pccl)) return false;
  make_bucket(pccl, b);
  swap(index_[b].first, pccl);
  return true;
}

PC_MAP_METHOD_DECL(iterator, find)(const key_type& k) {
  auto h = hasher_(k, seed_);
  auto b = h & (index_.size() - 1);
  auto mph = index_[b].first.minimal_perfect_hash(h);
  typename vector<vector<value_type>>::iterator ot = values_.begin() + b;
  typename vector<value_type>::iterator it = values_[b].begin() + mph;
  return make_flatten(&values_, ot, it); 
}
PC_MAP_METHOD_DECL(const_iterator, find)(const key_type& k) const {
  auto h = hasher_(k, seed_);
  auto b = h & (index_.size() - 1);
  auto mph = index_[b].first.minimal_perfect_hash(h);
  return make_flatten(&values_, values_.begin() + b, index_[b].second + mph); 
}

PC_MAP_METHOD_DECL(data_type&, operator[])(const key_type& k) {
  return insert(make_pair(k, data_type())).first->second;
}


}  // namespace cxxmph
#endif
