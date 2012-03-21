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
  dynamic_2bitset() : size_(0), fill_(false)  {}
  dynamic_2bitset(uint32_t size, bool fill = false)
      : size_(size), fill_(fill), data_(ceil(size / 4.0), ones()*fill) {
  }

  const uint8_t operator[](uint32_t i) const { return get(i); }
  const uint8_t get(uint32_t i) const { 
    assert(i < size());
    assert((i >> 2) < data_.size());
    return (data_[(i >> 2)] >> (((i & 3) << 1)) & 3);
  }
  uint8_t set(uint32_t i, uint8_t v) { 
    assert((i >> 2) < data_.size());
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
    other.data_.swap(data_);
  }
  void clear() { data_.clear(); size_ = 0; }
    
  uint32_t size() const { return size_; }
  static const uint8_t vmask[];
  const std::vector<uint8_t>& data() const { return data_; }
 private:
  uint32_t size_;
  bool fill_;
  std::vector<uint8_t> data_;
  const uint8_t ones() { return std::numeric_limits<uint8_t>::max(); }
};

static uint32_t nextpoweroftwo(uint32_t k) {
  if (k == 0) return 1;
  k--;
  for (int i=1; i<sizeof(uint32_t)*CHAR_BIT; i<<=1) k = k | k >> i;
  return k+1;
}
// Interesting bit tricks that might end up here:
// http://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
// Fast a % (k*2^t)
// http://www.azillionmonkeys.com/qed/adiv.html
// rank and select:
// http://vigna.dsi.unimi.it/ftp/papers/Broadword.pdf

struct Ranktable { static uint8_t get(uint8_t); };
static uint8_t rank64(uint64_t bits) {
  auto bytes = reinterpret_cast<const uint8_t*>(&bits);
  return Ranktable::get(bytes[0]) + Ranktable::get(bytes[1]) +
         Ranktable::get(bytes[2]) + Ranktable::get(bytes[3]);
};
  
}  // namespace cxxmph

#endif
