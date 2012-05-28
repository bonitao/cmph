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
using std::adjacent_find;
using std::greater;

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
  typedef pair<uint64_t, value_type> indexed_value_type;

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
  typedef uint64_t my_uint64_t;  // help macros
  inline int32_t index(const key_type& k) const;

  size_type bucket_count() const { return values_.size() - 48; }
  void rehash(size_type nbuckets /*ignored*/) { if (!pack(true)) exit(-1); }

 protected:  // mimicking STL implementation
  EqualKey equal_;

 private:
  bool pack(bool minimal = false);
  uint32_t n_;  // number of keys
  uint32_t m_;  // number of buckets == values_.size() - 48
  uint32_t seed_;
  typename seeded_hash<hasher>::hash_function hasher_;
  vector<indexed_value_type> values_;

  struct bfcr_is_empty {
    bfcr_is_empty() : c_(NULL) {}
    bfcr_is_empty(const vector<indexed_value_type>* c) : c_(c) {}
    bool operator()(
        typename vector<indexed_value_type>::const_iterator it) const {
      if (it == c_->end()) return false;
      fprintf(stderr, "Checking %llx for empty at pos %u.\n", it->first, uint32_t(it-c_->begin()));
      return (it->first & 1) == 0;
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
  
  bool present(uint64_t mph) const {
    return mph & 1ULL << 63;
  }
  void setpresent(uint64_t* mph, bool present) const {
    *mph &= ones() >> 1;
    *mph |= static_cast<uint64_t>(present) << 63;
    assert(this->present(*mph) == present);
  }
  uint64_t seed(uint64_t mph) const {
    return mph >> 48;
  }
  void setseed(uint64_t* mph, uint16_t seed) const {
    assert(present(*mph) == (seed & (1ULL << 63)));
    *mph &= ones() >> 16;
    *mph |= static_cast<uint64_t>(seed) << 48;
  }
  uint64_t pos(uint64_t mph, uint16_t index) const {
    assert(index < 6);
    return (mph >> (index << 3)) & 255;
  }
  void setpos(uint64_t* mph, uint8_t index, uint8_t pos) const {
    assert(pos < 255);
    assert(index < 6);
    *mph &= ones() ^ ((ones() & 255) << (index << 3));
    *mph |= static_cast<uint64_t>(pos) << (index << 3);
  }
  uint64_t reseed(const h128& h, uint32_t seed) const {
    return (h[1] >> (seed & 63)) ^ 
           (h[2] >> ((seed >> 5) & 63)) ^
	   (h[3] >> ((seed >> 10) & 63));
  }

  my_uint64_t create_mph(uint32_t index, const vector<indexed_value_type>& values) const;
  uint64_t eval_mph(const h128& h, uint64_t mph) const {
    return pos(mph, reseed(h, seed(mph)) & 7);
  }
  my_int32_t find_insert_index(const value_type& x, const vector<indexed_value_type>& values) const;
};


BFCR_MAP_INLINE_METHOD_DECL(iterator, begin)() { return make_hollow(values_.begin()); }
BFCR_MAP_INLINE_METHOD_DECL(iterator, end)() { return make_solid(values_.end()); }
BFCR_MAP_INLINE_METHOD_DECL(const_iterator, begin)() const { return make_hollow(values_.begin()); }
BFCR_MAP_INLINE_METHOD_DECL(const_iterator, end)() const { return make_solid(values_.end()); }

BFCR_MAP_TMPL_SPEC BFCR_MAP_CLASS_SPEC::bfcr_map()
    : n_(0), m_(8), seed_(random()), values_(m_+48), is_empty_(&values_) {
}

BFCR_MAP_METHOD_DECL(my_int32_t, find_insert_index)(
    const value_type& x,
    const vector<indexed_value_type>& values) const {
  auto m = values.size() - 48;
  auto insert_index = -1;
  auto h = hasher_.hash128(x.first, seed_);
  auto it = values.begin() + (h[0] % m);
  for (int i = 0; i < 48; ++it, ++i) {
    if (present(it->first)) continue;
    insert_index = it - values.begin();
    break;
  }
  // cerr << "Insertion index " << (h[0] % m) << "+" << (insert_index - (h[0] % m));
  // cerr << " for key " << x.first << endl;
  return insert_index;
}

BFCR_MAP_METHOD_DECL(my_uint64_t, create_mph)(
    uint32_t index,
    const vector<indexed_value_type>& values) const {
  auto pos = values.begin() + index;
  vector<h128> hash;
  vector<uint32_t> offset;
  for (auto it = pos; it < pos + 48; ++it) {
    if (present(it->first) == 0) continue;
    auto h = hasher_.hash128(it->second.first, seed_);
    if ((h[0] % m_) != index) continue;
    hash.push_back(h);
    offset.push_back(it - pos);
  }
  assert(hash.size());
  // cerr << "Finding injection for " << hash.size() << " keys " << endl;

  // Just need to guarantee order
  uint64_t mph = 0;
  int16_t iterations = 1ULL << 14;
  while (iterations--) {
    bool success = true;
    mph = 0;
    setseed(&mph, random() & 63);
    setpresent(&mph, present(pos->first));
    vector<bool> used_rank(8);
    for (uint32_t i = 0; i < hash.size(); ++i) {
      int64_t rank = reseed(hash[i], seed(mph)) & 7; 
      if (rank >= 6) { success = false; break; }
      if (used_rank[rank]) { success = false; break; }
      used_rank[rank] = true;
      setpos(&mph, rank, offset[i]);
    }
    // cerr << "Success now: " << uint32_t(success) << endl;
    if (!success) continue;
    break;
  }
  if (iterations < 0) return -1;
  #ifndef NDEBUG
  for (auto it = pos; it < pos + 48; ++it) {
    if (!present(it->first)) continue;
    auto h = hasher_.hash128(it->second.first, seed_);
    if ((h[0] % m_) != index) continue;
    uint32_t offset = eval_mph(h, mph);
    assert(offset == it - pos);
  }
  #endif
  return mph;
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
  setpresent(&insert_position->first, true);
  
  uint64_t mph_seed = create_mph(h[0] % m_, values_);
  // cerr << "Found mph seed " << mph_seed << endl;
  if (mph_seed == static_cast<uint64_t>(-1)) {
    // already inserted, pack will take care of fixing mph
    //  cerr << "Packing at size " << size() << " occupation " << size()*1.0/bucket_count() << " because there is no mph" << endl;
    if (!pack()) exit(-1);
  } else {
    (values_.begin() + (h[0] % m_))->first = mph_seed;
    // cerr << "Set seed for pos " << (h[0] % m_) << endl;
    assert(present((values_.begin() + (h[0] % m_))->first));
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
    // cerr<< "Minimal pack for " << size() << " keys will use " << m_ << " buckets." << endl;
   }
  // cerr << "Running pack with " << n_ << " keys growing to size " << (m_+48) << endl;
  int iterations = 64;
  vector<indexed_value_type> new_values;
  while (iterations--) {
    // cerr << "Trying pack iteration " << iterations << endl;
    vector<indexed_value_type>(m_ + 48).swap(new_values);
    bool success = true; 
    seed_ = random();
    for (auto it = values_.begin(); it != values_.end(); ++it) {
      if (!present(it->first)) continue;
      auto insert_index = find_insert_index(it->second, new_values);
      if (insert_index == -1) {
        // cerr << "Find index pack failure" << endl;
        success = false;
      }
      auto insert_position = new_values.begin() + insert_index;

      auto h = hasher_.hash128(it->second.first, seed_);
      setpresent(&insert_position->first, true);
      insert_position->second = it->second;

      auto mph_seed = create_mph(h[0] % m_, new_values);
      if (mph_seed == static_cast<uint64_t>(-1)) {
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
  // cerr << "index: " << vit - values_.begin() << endl;
  if (__builtin_expect(present(vit->first), true)) { 
    // cerr << "value is present " << endl;
    if (__builtin_expect(equal_(k, vit->second.first), true)) {
    // cerr << "value is equal " << endl;
      return this->make_solid((vit));
    }
  } else {
   // cerr << "value is not present" << endl;
  }
  return end();
}

BFCR_MAP_INLINE_METHOD_DECL(const_iterator, find)(const key_type& k) const {
  auto vit = values_.begin() + index(k);
  if (__builtin_expect((vit->first >= 0 && vit->second.first ==k), true)) return make_solid(vit);
  return end();
}

BFCR_MAP_INLINE_METHOD_DECL(my_int32_t, index)(const key_type& k) const {
  auto h = hasher_.hash128(k, seed_);
  auto pos = h[0] & (m_-1);
  auto it = values_.begin() + pos;
  __builtin_prefetch(&(it->second.first));
  auto index = reseed(h, seed(it->first)) & 7;
  auto offset = (it->first >> (index << 3)) & 255;
  // cerr << "Key " << k << " is at pos " << pos << " offset " << offset << endl;
  // cerr << "Search index for key " << k << " is " << (h[0] % m_) << "+" << offset << endl;
  return pos + offset;
}

BFCR_MAP_METHOD_DECL(data_type&, operator[])(const key_type& k) {
  return insert(make_pair(k, data_type())).first->second;
}

BFCR_MAP_METHOD_DECL(void_type, erase)(iterator pos) {
  setpresent(&(pos.it_->first), false);
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
