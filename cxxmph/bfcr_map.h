#ifndef __CXXMPH_BFCR_MAP_H__
#define __CXXMPH_BFCR_MAP_H__

// On collision, find an empty position up to k positions ahead and insert.
// Then store the set S of positions p \in [0;k] for the c keys that have
// collided. The higher k, the bigger occupation we can have. The lower k, the
// more likely it is to find a function h(v, seed) that maps [v]->S.

#include <algorithm>
#include <bitset>
#include <unordered_map>  // for std::hash
#include <iostream>
#include <vector>

#include "mph_bits.h"
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
#define BFCR_MAP_TMPL_SPEC template <class Key, class Data, class HashFcn, class EqualKey, class Alloc>
#define BFCR_MAP_CLASS_SPEC bfcr_map<Key, Data, HashFcn, EqualKey, Alloc>
#define BFCR_MAP_METHOD_DECL(r, m) BFCR_MAP_TMPL_SPEC typename BFCR_MAP_CLASS_SPEC::r BFCR_MAP_CLASS_SPEC::m
#define BFCR_MAP_INLINE_METHOD_DECL(r, m) BFCR_MAP_TMPL_SPEC inline typename BFCR_MAP_CLASS_SPEC::r BFCR_MAP_CLASS_SPEC::m

template <class Key, class Data, class HashFcn = std::hash<Key>, class EqualKey = std::equal_to<Key>, class Alloc = std::allocator<Data> >
class bfcr_map {
 public:
  typedef Key key_type;
  typedef Data data_type;
  typedef pair<Key, Data> value_type;
  typedef HashFcn hasher;
  typedef EqualKey key_equal;

 private:
  struct bfcr_is_empty;
  typedef pair<uint16_t, value_type> indexed_value_type;

 public:
  typedef typename vector<indexed_value_type>::pointer pointer;
  typedef typename vector<indexed_value_type>::reference reference;
  typedef typename vector<indexed_value_type>::const_reference const_reference;
  typedef typename vector<indexed_value_type>::size_type size_type;
  typedef typename vector<indexed_value_type>::difference_type difference_type;

  typedef iterator_second<hollow_iterator_base<typename vector<indexed_value_type>::iterator, bfcr_is_empty>> iterator;
  typedef iterator_second<hollow_iterator_base<typename vector<indexed_value_type>::const_iterator, bfcr_is_empty>> const_iterator;

  // For making macros simpler.
  typedef void void_type;
  typedef bool bool_type;
  typedef pair<iterator, bool> insert_return_type;

  bfcr_map();

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
  typedef int32_t my_int32_t;  // help macros
  inline int32_t index(const key_type& k) const;

  size_type bucket_count() const { return values_.size() - 8; }
  void rehash(size_type nbuckets /*ignored*/) { if (!pack(true)) exit(-1); }

 protected:  // mimicking STL implementation
  EqualKey equal_;

 private:
  bool pack(bool minimal = false);
  uint32_t n_;  // number of keys
  uint32_t m_;  // number of buckets == values_.size() - 8
  uint32_t seed_;
  typename seeded_hash<hasher>::hash_function hasher_;
  vector<indexed_value_type> values_;

  struct bfcr_is_empty {
    bfcr_is_empty() : c_(NULL) {}
    bfcr_is_empty(const vector<indexed_value_type>* c) : c_(c) {}
    bool operator()(
        typename vector<indexed_value_type>::const_iterator it) const {
      if (it == c_->end()) return false;
      fprintf(stderr, "Checking %x for empty at pos %u.\n", it->first, uint32_t(it-c_->begin()));
      return (it->first & (3ULL << 14)) == 0;
    }
   private:
    const vector<indexed_value_type>* c_;
  };
  bfcr_is_empty is_empty_;
  template <class ittype> inline auto make_hollow(ittype it) const ->
      iterator_second<hollow_iterator_base<ittype, bfcr_is_empty>> {
    return make_iterator_second(hollow_iterator_base<ittype, bfcr_is_empty>(it, is_empty_, false));
  }
  template <class ittype> inline auto make_solid(ittype it) const ->
      iterator_second<hollow_iterator_base<ittype, bfcr_is_empty>> {
    return make_iterator_second(hollow_iterator_base<ittype, bfcr_is_empty>(it, is_empty_));
  }

  uint8_t count(typename vector<indexed_value_type>::const_iterator it) const {
    return it->first >> 14;
  }
  void setcount(typename vector<indexed_value_type>::iterator it, uint8_t count) const {
    it->first &= static_cast<uint16_t>(ones()) >> 2;
    it->first |= static_cast<uint16_t>(count) << 14;
    assert(this->count(it) == count);
  }

  my_int32_t create_mph(uint32_t index, const vector<indexed_value_type>& values) const;
  my_int32_t find_insert_index(const value_type& x, const vector<indexed_value_type>& values) const;
};


BFCR_MAP_INLINE_METHOD_DECL(iterator, begin)() { return make_hollow(values_.begin()); }
BFCR_MAP_INLINE_METHOD_DECL(iterator, end)() { return make_solid(values_.end()); }
BFCR_MAP_INLINE_METHOD_DECL(const_iterator, begin)() const { return make_hollow(values_.begin()); }
BFCR_MAP_INLINE_METHOD_DECL(const_iterator, end)() const { return make_solid(values_.end()); }

BFCR_MAP_TMPL_SPEC BFCR_MAP_CLASS_SPEC::bfcr_map()
    : n_(0), m_(8), seed_(random()), values_(m_+8), is_empty_(&values_) {
}

BFCR_MAP_METHOD_DECL(my_int32_t, find_insert_index)(
    const value_type& x,
    const vector<indexed_value_type>& values) const {
  auto m = values.size() - 8;
  auto insert_index = -1;
  auto h = hasher_.hash128(x.first, seed_);
  auto it = values.begin() + (h[0] % m);
  for (int i = 0; i < 8; ++it, ++i) {
    if (count(it) != 0) continue;
    insert_index = it - values.begin();
    break;
  }
  // cerr << "Insertion index " << (h[0] % m) << "+" << (insert_index - (h[0] % m)) << " (";
  // if (insert_index != -1) {
  // cerr << static_cast<uint32_t>(count(values.begin()+insert_index));
  // cerr << " count) for key " << x.first << endl;
  // }
  return insert_index;
}

BFCR_MAP_METHOD_DECL(my_int32_t, create_mph)(
    uint32_t index,
    const vector<indexed_value_type>& values) const {
  auto pos = values.begin() + index;
  vector<pair<uint32_t, uint32_t>> index_offset;
  for (auto it = pos; it < pos + 8; ++it) {
    if (count(it) == 0) continue;
    auto h = hasher_.hash128(it->second.first, seed_);
    if ((h[0] % m_) != index) continue;
    index_offset.push_back(make_pair(h[1], it - pos));
  }
  assert(index_offset.size());
  // cerr << "Finding injection for " << index_offset.size() << " keys " << endl;

  uint16_t iterations = 1ULL << 14;
  while (iterations--) {
    bool success = true;
    uint16_t mph_seed = (random() & (static_cast<uint16_t>(ones()) >> 2)) | (static_cast<uint16_t>(count(pos)) << 14);
    for (auto it = index_offset.begin(); it != index_offset.end(); ++it) {
      auto hp = reseed2(it->first, mph_seed) % 8;
      if (hp != it->second) { success = false; break; }
      else { /*cerr << "Succeded at offset "<< it->second << endl;*/ }
    }
    if (success) { 
      #ifndef NDEBUG
      for (auto it = pos; it < pos + 8; ++it) {
        if (count(it) == 0) continue;
        auto h = hasher_.hash128(it->second.first, seed_);
        if ((h[0] % m_) != index) continue;
        auto hp = reseed2(h[1], mph_seed) % 8;
        assert((values.begin() + (h[0] % m_) + hp)->second.first == it->second.first);
      }
      #endif
      return mph_seed;
    }
  }
  return -1;
}

BFCR_MAP_METHOD_DECL(insert_return_type, insert)(const value_type& x) {
  auto it = find(x.first);
  auto it_end = end();
  if (it != it_end) {
    // cerr << "Key " << x.first << " already present. " << endl;
    return make_pair(it, false);
  }

  auto insert_index = -1;
  while (insert_index == -1) {
    insert_index = find_insert_index(x, values_);
    if (insert_index == -1) { 
      // cerr << "Packing at size " << size() << " occupation " << size()*1.0/bucket_count() << " because there is no index" << endl;
      if (!pack()) exit(-1);
    }
  }

  auto insert_position = values_.begin() + insert_index;
  insert_position->second = x;
  auto h = hasher_.hash128(x.first, seed_);
  setcount(values_.begin() + (h[0] % m_), 1);
  setcount(insert_position, 1);
  
  uint16_t mph_seed = create_mph(h[0] % m_, values_);
  // cerr << "Found mph seed " << mph_seed << endl;
  if (mph_seed == static_cast<uint16_t>(-1)) {
    // already inserted, pack will take care of fixing mph
    //  cerr << "Packing at size " << size() << " occupation " << size()*1.0/bucket_count() << " because there is no mph" << endl;
    if (!pack()) exit(-1);
  } else {
    (values_.begin() + (h[0] % m_))->first= mph_seed;
  }
  it = find(x.first);
  assert(it != end());
  ++n_;
  return make_pair(it, true);
}

BFCR_MAP_METHOD_DECL(bool_type, pack)(bool minimal) {
  m_ *= 2;
  if (minimal) {
    m_ = nextpoweroftwo(size()*2);  // growth
    cerr<< "Minimal pack for " << size() << " keys will use " << m_ << " buckets." << endl;
   }
  // cerr << "Running pack with " << n_ << " keys growing to size " << (m_+8) << endl;
  int iterations = 64;
  vector<indexed_value_type> new_values;
  while (iterations--) {
    // cerr << "Trying pack iteration " << iterations << endl;
    vector<indexed_value_type>(m_ + 8).swap(new_values);
    bool success = true; 
    seed_ = random();
    for (auto it = values_.begin(); it != values_.end(); ++it) {
      if (count(it) == 0) continue;
      auto insert_index = find_insert_index(it->second, new_values);
      if (insert_index == -1) {
        // cerr << "Find index pack failure" << endl;
        success = false;
      }
      auto insert_position = new_values.begin() + insert_index;

      auto h = hasher_.hash128(it->second.first, seed_);
      setcount(new_values.begin() + (h[0] % m_), 1);
      setcount(insert_position, 1);
      insert_position->second = it->second;

      auto mph_seed = create_mph(h[0] % m_, new_values);
      if (mph_seed == -1) {
        // cerr << "Create mph pack failure" << endl;
        success = false;
      }
      (new_values.begin() + (h[0] % m_))->first = mph_seed;
    }
    if (success) break;
  }
  if (iterations == -1) return false;
  new_values.swap(values_);
  return true;
}

BFCR_MAP_METHOD_DECL(iterator, find)(const key_type& k) {
  auto vit = values_.begin() + index(k);
  if (__builtin_expect(count(vit), true)) { 
    if (equal_(k, vit->second.first)) return this->make_solid((vit));
  }
  return end();
}

BFCR_MAP_INLINE_METHOD_DECL(const_iterator, find)(const key_type& k) const {
  auto h = hasher_.hash128(k, seed_);
  auto pos = h[0] & (m_-1);
  // __builtin_prefetch(&(*(values_.begin() + pos)));
  auto offset = reseed2(h[1], (values_.begin() + pos)->first) & 7;
  auto vit = values_.begin() + pos + offset;
  if (vit->second.first != k) return end();
  return make_solid(vit);
  // auto e = static_cast<uint32_t>(equal_(k, vit->second.first)) - 1;
  // return make_solid(values_.begin() + branch_free_end(pos + offset, e));
  // if (__builtin_expect(count(vit), true)) { 
  // if (equal_(k, vit->second.first)) return make_solid((vit));
  // }
  // return end();
}

BFCR_MAP_INLINE_METHOD_DECL(my_int32_t, index)(const key_type& k) const {
  auto h = hasher_.hash128(k, seed_);
  auto pos = h[0] & (m_-1);
  auto offset = reseed2(h[1], (values_.begin() + pos)->first) & 7;
  // cerr << "Search index for key " << k << " is " << (h[0] % m_) << "+" << offset << endl;
  return pos + offset;
}

BFCR_MAP_METHOD_DECL(data_type&, operator[])(const key_type& k) {
  return insert(make_pair(k, data_type())).first->second;
}

BFCR_MAP_METHOD_DECL(void_type, erase)(iterator pos) {
  setcount(pos.it_, 0);
  pos.it_->second = value_type();
  --n_;
}

BFCR_MAP_METHOD_DECL(void_type, erase)(const key_type& k) {
  iterator it = find(k);
  if (it == end()) return;
  erase(it);
}


}  // namespace cxxmph
#endif
