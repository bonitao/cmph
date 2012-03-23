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
  uint32_t n_;  // number of keys
  uint32_t seed_;
  seeded_hash<hasher> hasher_;

  struct superblock {
    uint32_t offset;
    uint8_t block_rank[1 + 6/2];  // first is seed, max 16 keys per block
    std::bitset<64> block[7];  // use 4 bits and a table to weight latter blocks more
  };

  vector<superblock> superblock_;
  vector<bool> present_;  // not really needed, but easier for initial code 
};

PC_MAP_TMPL_SPEC MPH_MAP_CLASS_SPEC::perfect_cuckoo_map()
    : size_(0), m_(2), superblock_(m_) {
}

PC_MAP_METHOD_DECL(insert_return_type, insert)(const value_type& x) {
  auto it = find(x.first);
  if (it != end()) return make_pair(it, false);

  ++n_;
  uint32_t p[3];
  find_block(hasher_(k, seed), superblock_, p);
  auto bit = superblock_[p[0]].block[p[1]][p[2]];
  if (bit) {
    values_.push_back(x);
    present_.push_back(true);
    pack(false);
  } else {
    bit = true;
    auto rank = get_nibble(r, p[1] & 1);
    set_nibble(r, p[1] & 1, rank + 1);
    if (rank + 1 > 15 || 
        (p[0] + 1 < superblock_.size() &&
         sum_nibbles() + superblock_[p[0]].offset > superblock_[p[0]+1].offset) {
      values_.push_back(x);
      present_.push_back(true);
      pack(false);
    } else {
      uint32_t pos = findpos(seed_, x, superblock_); 
      for (int i = superblock_[p[0] + 1].offset - 1; i > pos; --i) {
        values_[i] = values_[i-1];
      }
      values_[pos] = x;
    }
  }
}

PC_MAP_METHOD_DECL(void, findblock)(
    uint32_t h, const vector<superblock>& superblocks, uint32_t *p) const {
  (*p)[0] = h & (superblocks.size() - 1);
  (*p)[1] = disperse_nibble[h >> 28];
  (*p)[2] = (h & (ones() >> 4) >> (32 - 4 - 5));
}

PC_MAP_METHOD_DECL(uint32_t, findpos)(
   uint32_t seed, const key_type& x,
   const vector<superblock> superblocks) const {
  uint32_t p[3];
  find_block(hasher_(x, seed), superblock, p);
  const superblock& s(new_superblock[p[0]]);
  return sum_nibbles(s.rank[p[1]/2]) + p[2];
}
  
PC_MAP_METHOD_DECL(bool, pack)(bool minimal) {
  vector<value_type> new_values(values_.size()*2);
  vector<bool> new_present(values_.size()*2);
  vector<superblock> new_superblock(
       max(nextpoweroftwo(values_.size())/64, 2));
  auto iterations = 100;
  seed = 0;
  while (iterations) {
    seed = random();
    for (int i = 0; i < values_.size(); ++i) {
      if (!present_[i]) continue;
      uint32_t p[3];
      find_block(hasher_(k, seed), new_superblock, p);
      auto bit = new_superblock[p[0]].block[p[1]][p[2]];
      if (bit) {
        --iterations;
        break;
      } else {
        bit = true;
        uint8_t* r = new_superblock[p[0]].rank[p[1]/2];
        set_nibble(r, p[1] & 1, get_nibble(r, p[1] & 1) + 1);
      }
    }
  }
  if (iterations == 0) return false;
  uint32_t offset = 0;
  for (int i = 0; i < new_superblock_.size(); ++i) {
    superblock& s(new_superblock[i]);
    s.offset = offset;
    if (minimal) offset += sum_nibbles(s.rank, 8);
    else offset += nextpoweroftwo(sum_nibbles(s.rank, 8));
  }
  for (int i = 0; i < values_.size(); ++i) {
    if (!present_[i]) continue;
    uint32_t pos = findpos(seed, values_[i], new_superblock);
    new_values[pos] = values_[i];
    new_present[pos] = true;
  }
  seed_ = seed;
  values_.swap(new_values);
  present_.swap(new_present);
}

PC_MAP_METHOD_DECL(iterator, find)(const key_type& k) {
  return make_iterator(
     values.begin() + findpos(seed_, k, superblock_));
}
PC_MAP_METHOD_DECL(const_iterator, find)(const key_type& k) const {
  return make_iterator(
     values.begin() + findpos(seed_, k, superblock_));
}
