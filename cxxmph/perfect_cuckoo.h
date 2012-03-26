#ifndef __CXXMPH_PERFECT_CUCKOO_H__
#define __CXXMPH_PERFECT_CUCKOO_H__

#include <bitset>
#include <vector>

namespace cxxmph {

using std::pair;
using std::make_pair;
using std::unordered_map;
using std::vector;

// Save on repetitive typing.
#define PC_MAP_TMPL_SPEC template <class Key, class Data, class HashFcn, class EqualKey, class Alloc>
#define PC_MAP_CLASS_SPEC mph_map<Key, Data, HashFcn, EqualKey, Alloc>
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

  typedef hollow_iterator<std::vector<value_type>> iterator;
  typedef hollow_const_iterator<std::vector<value_type>> const_iterator;

  // For making macros simpler.
  typedef void void_type;
  typedef bool bool_type;
  typedef pair<iterator, bool> insert_return_type;

  perfect_cuckoo_map();
  ~perfect_cuckoo_map();

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
  iterator find(const key_type& k) { return slow_find(k, index_.minimal_perfect_hash(k)); }
  const_iterator find(const key_type& k) const { return slow_find(k, index_.minimal_perfect_hash(k)); };
  data_type& operator[](const key_type &k);
  const data_type& operator[](const key_type &k) const;

  size_type bucket_count() const { return values_.size(); }
  void rehash(size_type nbuckets /*ignored*/) { pack(true); }

 private:
  bool pack(bool minimal = false);
  bool pack_bucket(bool minimal = false);

  uint32_t n_;  // number of keys
  uint32_t seed_;
  seeded_hash<hasher> hasher_;

  typedef vector<vector<value_type> > values_type;
  values_type values_;
  typedef vector<pair<perfect_cuckoo_cache_line, vector<value_type>::iterator>> index_type; 
  index_type index_; 
};

  
PC_MAP_TMPL_SPEC PC_MAP_CLASS_SPEC::perfect_cuckoo_map()
    : n_(0), seed_(random()), index_(1) {
}

PC_MAP_METHOD_DECL(insert_return_type, insert)(const value_type& x) {
  auto h = hasher_(x, seed_);
  auto b = h & (index_.size() - 1);
  values_[b].push_back(x);
  ++n_;
  if (!index_[b].first.insert(h)) {
    if (!pack_bucket(b)) {
      pack();
    }
  }
}

PC_MAP_METHOD_DECL(bool, pack)(bool minimal) {
  int iterations = 255;
  uint32_t index_size = nextpoweroftwo(size());
  uint32_t index_size /= perfect_cuckoo_cache_line::good_capacity();
  index_type index(index_size);
  vector<vector<uint32_t>> hashes(index_size);
  uint32_t seed = 0;
  while (iterations) {
    auto success = true;
    seed = random();
    for (auto it = begin(), i = 0; it != end(); ++it, ++i) {
      auto h = hasher_(*it, seed);
      auto b = h & (index.size() - 1);
      hashes[b].push_back(h);
    }
    for (uint32_t i = 0; i < index_size; ++i) {
      if (!make_pccl(hashes[i], &index[i].first)) {
        success = false;
        break;
      }
    }
    if (!success) continue;
    vector<vector<uint32_t>>.swap(hashes);
    values_type values(index_size);
    for (int i = 0; i < index_size; ++i) values[i].resize(index_[i].first.size());
    for (auto it = begin(), i = 0; it != end(); ++it, ++i) {
      auto h = hasher_(*it, seed);
      auto b = h & (index.size() - 1);
      auto mph = index[b].first.minimal_perfect_hash(h);
      values[b][mph] = *it; 
    }
    values.swap(values_);
    break;
  }
}

PC_MAP_METHOD_DECL(bool, pack_bucket)(uint32_t b) {
  uint32_t seed = random();
  perfect_cuckoo_cache_line pccl;
  int iterations = 255;
  while (iterations--) {
    pccl.set_seed(random());
    pccl.clear();
    bool success = true;
    for (auto it = values_[b].begin(), end = values_[b].end();
         it != end; ++it) {
      auto h = hasher_(*it, seed_);
      if (!pccl.insert(h)) {
        success = false;
        break;
      }
    }
    if (success) {
      vector<value_type> v(values_[b].size());
      for (auto it = values_[b].begin(), end = values_[b].end();
         it != end; ++it) {
        auto h = hasher_(*it, seed_);
        auto i = pccl.minimal_perfect_hash(h);
        swap(v[i], *it);
      }
      swap(values_[b], v);
      swap(index_[b].first, pccl);
      index_[b].second = values_[b].begin();
      break;
    }
  }
  return iterations >= 0;
}

PC_MAP_METHOD_DECL(iterator, find)(const key_type& k) {
  auto h = hasher_(x, seed_);
  auto b = h & (index_.size() - 1);
  auto mph = index_[b].minimal_perfect_hash(h);
  return index_[b].second + mph;
}
PC_MAP_METHOD_DECL(const_iterator, find)(const key_type& k) const {
  auto h = hasher_(x, seed_);
  auto b = h & (index_.size() - 1);
  auto mph = index_[b].minimal_perfect_hash(h);
  return index_[b].second + mph;
}
