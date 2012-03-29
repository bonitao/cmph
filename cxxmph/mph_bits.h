#ifndef __CXXMPH_MPH_BITS_H__
#define __CXXMPH_MPH_BITS_H__

#include <stdint.h>  // for uint32_t and friends
#include <limits>

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
  dynamic_2bitset();
  ~dynamic_2bitset();
  dynamic_2bitset(uint32_t size, bool fill = false);

  const uint8_t operator[](uint32_t i) const { return get(i); }
  const uint8_t get(uint32_t i) const { 
    assert(i < size());
    assert((i >> 2) < data_.size());
    return (data_[(i >> 2)] >> (((i & 3) << 1)) & 3);
  }
  void set(uint32_t i, uint8_t v) { 
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
  for (uint32_t i=1; i<sizeof(uint32_t)*CHAR_BIT; i<<=1) k = k | k >> i;
  return k+1;
}
// Interesting bit tricks that might end up here:
// http://graphics.stanford.edu/~seander/bithacks.html#ZeroInWord
// Fast a % (k*2^t)
// http://www.azillionmonkeys.com/qed/adiv.html
// rank and select:
// http://vigna.dsi.unimi.it/ftp/papers/Broadword.pdf

struct Ranktable { static uint8_t get(uint8_t); };

// From sux-0.7
#define ONES_STEP_4 ( 0x1111111111111111ULL )
#define ONES_STEP_8 ( 0x0101010101010101ULL )
static uint8_t rank64(uint64_t x) {
  register uint64_t byte_sums = x - ( ( x & 0xa * ONES_STEP_4 ) >> 1 );
  byte_sums = ( byte_sums & 3 * ONES_STEP_4 ) + ( ( byte_sums >> 2 ) & 3 * ONES_STEP_4 );
  byte_sums = ( byte_sums + ( byte_sums >> 4 ) ) & 0x0f * ONES_STEP_8;
  return byte_sums * ONES_STEP_8 >> 56;
};

static const uint64_t ones() { return std::numeric_limits<uint64_t>::max(); }

static const uint8_t most_significant_bit(uint64_t v, uint8_t r) {
  unsigned int s;      // Output: Resulting position of bit with rank r [1-64]
  uint64_t a, b, c, d; // Intermediate temporaries for bit count.
  unsigned int t;      // Bit count temporary.

  // Do a normal parallel bit count for a 64-bit integer,                     
  // but store all intermediate steps.                                        
  // a = (v & 0x5555...) + ((v >> 1) & 0x5555...);
  a =  v - ((v >> 1) & ~0UL/3);
  // b = (a & 0x3333...) + ((a >> 2) & 0x3333...);
  b = (a & ~0UL/5) + ((a >> 2) & ~0UL/5);
  // c = (b & 0x0f0f...) + ((b >> 4) & 0x0f0f...);
  c = (b + (b >> 4)) & ~0UL/0x11;
  // d = (c & 0x00ff...) + ((c >> 8) & 0x00ff...);
  d = (c + (c >> 8)) & ~0UL/0x101;
  t = (d >> 32) + (d >> 48);
  // Now do branchless select!                                                
  s  = 64;
  // if (r > t) {s -= 32; r -= t;}
  s -= ((t - r) & 256) >> 3; r -= (t & ((t - r) >> 8));
  t  = (d >> (s - 16)) & 0xff;
  // if (r > t) {s -= 16; r -= t;}
  s -= ((t - r) & 256) >> 4; r -= (t & ((t - r) >> 8));
  t  = (c >> (s - 8)) & 0xf;
  // if (r > t) {s -= 8; r -= t;}
  s -= ((t - r) & 256) >> 5; r -= (t & ((t - r) >> 8));
  t  = (b >> (s - 4)) & 0x7;
  // if (r > t) {s -= 4; r -= t;}
  s -= ((t - r) & 256) >> 6; r -= (t & ((t - r) >> 8));
  t  = (a >> (s - 2)) & 0x3;
  // if (r > t) {s -= 2; r -= t;}
  s -= ((t - r) & 256) >> 7; r -= (t & ((t - r) >> 8));
  t  = (v >> (s - 1)) & 0x1;
  // if (r > t) s--;
  s -= ((t - r) & 256) >> 8;
  s = 65 - s;
  return s;
}

inline uint64_t rotl64 ( uint64_t x, int8_t r )
{
  return (x << r) | (x >> (64 - r));
}

inline uint32_t rotl32 (uint32_t x, int8_t r) {
  return (x << r) | (x >> (32 - r));
}
inline uint32_t fmix(uint32_t h) {
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  return h;
}
inline uint32_t reseed32(uint32_t h, uint32_t seed) {
  uint32_t c1 = 0xcc9e2d51;
  uint32_t c2 = 0x1b873593;

  uint32_t h1 = seed;
  uint32_t k1 = h;

  k1 *= c1;
  k1 *= c2;

  h1 ^= k1;
  h1 = rotl32(h1,13); 
  h1 = h1*5+0xe6546b64;
  h1 = fmix(h1);
  return h1;
}
  
}  // namespace cxxmph

#endif
