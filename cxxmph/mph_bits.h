#ifndef __CXXMPH_MPH_BITS_H__
#define __CXXMPH_MPH_BITS_H__

#include <stdint.h>  // for uint32_t and friends

#include <array>
#include <cassert>
#include <climits>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>
#include <utility>

namespace cxxmph {

class dynamic_2bitset {
 public:
  dynamic_2bitset() : fill_(false)  {}
  dynamic_2bitset(uint32_t size, bool fill = false)
      : size_(size), fill_(fill), data_(ceil(size / 4.0), ones()*fill) {
  }

  const uint8_t operator[](uint32_t i) const { return get(i); }
  uint8_t get(uint32_t i) const { 
    return (data_[(i >> 2)] >> (((i & 3) << 1)) & 3);
  }
  uint8_t set(uint32_t i, uint8_t v) { 
    data_[(i >> 2)] |= ones() ^ dynamic_2bitset::vmask[i & 3];
    data_[(i >> 2)] &= ((v << ((i & 3) << 1)) | dynamic_2bitset::vmask[i & 3]);
    assert(v <= 3);
    assert(get(i) == v);
  }
  void resize(uint32_t size) {
    size_ = size;
    data_.resize(size >> 2, fill_*ones());
  }
  void swap(dynamic_2bitset& other) {
    std::swap(other.size_, size_);
    std::swap(other.fill_, fill_);
    std::swap(other.data_, data_);
  }
  void clear() { data_.clear(); }
    
  uint32_t size() const { return size_; }
  static const uint8_t vmask[];
 private:
  uint32_t size_;
  bool fill_;
  std::vector<uint8_t> data_;
  uint8_t ones() { return std::numeric_limits<uint8_t>::max(); }
};

static void set_2bit_value(uint8_t *d, uint32_t i, uint8_t v) {
  d[(i >> 2)] &= ((v << ((i & 3) << 1)) | dynamic_2bitset::vmask[i & 3]);
}
static uint32_t get_2bit_value(const uint8_t* d, uint32_t i) {
  return (d[(i >> 2)] >> (((i & 3) << 1)) & 3);
}
static uint32_t nextpoweroftwo(uint32_t k) {
  if (k == 0) return 1;
  k--;
  for (int i=1; i<sizeof(uint32_t)*CHAR_BIT; i<<=1) k = k | k >> i;
  return k+1;
}

// Interesting bit tricks that might end up here:
// http://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
  
}  // namespace cxxmph

#endif
