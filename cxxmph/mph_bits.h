#ifndef __CXXMPH_MPH_BITS_H__
#define __CXXMPH_MPH_BITS_H__

#include <stdint.h>  // for uint32_t and friends
#include <cassert>
#include <climits>
#include <cstdio>
#include <cstring>
#include <limits>
#include <vector>

namespace cxxmph {

class dynamic_2bitset {
 public:
  dynamic_2bitset() : data_(NULL), size_(0), one_initialized_(false)  {}
  dynamic_2bitset(uint32_t size, bool one_initialized = false)
      : data_(NULL), size_(0), one_initialized_(one_initialized) {
    resize(size);
  }
  ~dynamic_2bitset() { delete [] data_; }

  const uint8_t operator[](uint32_t i) const { return get(i); }
  uint8_t get(uint32_t i) const { 
    return (data_[(i >> 2)] >> (((i & 3) << 1)) & 3);
  }
  uint8_t set(uint32_t i, uint8_t v) { 
    uint8_t sf = ((v << ((i & 3) << 1)) | dynamic_2bitset::vmask[i & 3]);
    fprintf(stderr, "v %d sf %d\n", v, sf);
    data_[(i >> 2)] &= ((v << ((i & 3) << 1)) | dynamic_2bitset::vmask[i & 3]);
    assert(get(i) == v);
  }
  void resize(uint32_t size) {
    uint8_t* new_data = new uint8_t[size << 2];
    assert(one_initialized_);
    assert(one_initialized_ * ones() == ones());
    memset(new_data, one_initialized_*ones(), size << 2);
    assert(new_data[0] == ones());
    uint8_t* old_data_ = data_;
    for (int i = 0; i < size_; ++i) { 
      data_ = old_data_;
      auto v = get(i);
      data_ = new_data;
      set(i, v);
    }
    size_ = size;
    delete [] old_data_;
    data_ = new_data;
    assert(data_[0] == ones());
    assert(get(0) == 3);
  }
  static const uint8_t vmask[];
 private:
  uint8_t* data_;
  uint32_t size_;
  bool one_initialized_;
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
  
}  // namespace cxxmph

#endif
