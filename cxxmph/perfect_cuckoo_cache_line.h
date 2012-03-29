#ifndef __CXXMPH_PERFECT_CUCKOO_CACHE_LINE_H__
#define __CXXMPH_PERFECT_CUCKOO_CACHE_LINE__

#include <cmath>

#include "mph_bits.h"

namespace cxxmph {

class perfect_cuckoo_cache_line {
 public:
  perfect_cuckoo_cache_line() : seed_(0) { clear(); }
  void set_seed(uint32_t seed) { seed_ = seed; }
  const uint32_t seed() const { return seed_; }
  
  uint8_t size() const { return rank((sizeof(rank_)*8) - 1); }
  static uint8_t max_capacity() { return sizeof(select_)*8; }
  static uint8_t good_capacity() { return floor(sqrt(sizeof(rank_)*8)); }

  uint8_t perfect_hash(uint32_t h) const;
  uint8_t minimal_perfect_hash(uint32_t h) const;
  bool insert(uint32_t h);

  void clear() {
    memset(rank_, 0, sizeof(rank_));
    select_ = 0;
  }
 private:
  uint16_t rank(uint16_t pos) const;
  uint8_t next_empty() const;
  uint64_t rank_[4];  // 44
  uint64_t select_;  // 12
  uint32_t seed_;  // 4
};

uint8_t perfect_cuckoo_cache_line::minimal_perfect_hash(uint32_t h) const {
  h = reseed32(h, seed_);
  uint16_t bitpos = h & (sizeof(rank_)*8 - 1);
  // uint8_t wordpos = bitpos >> 6; // 6 == log(sizeof(uint64_t)*8) == log(64)
  // uint8_t inwordpos = bitpos & (sizeof(uint64_t)*8 - 1);
  // fprintf(stderr, "Searching %llu at bitpos %d wordpos %d inwordpos %d\n",
  //                 h, bitpos, wordpos, inwordpos);
  return rank(bitpos) - 1; 
}

uint8_t perfect_cuckoo_cache_line::perfect_hash(uint32_t h) const {
  // fprintf(stderr, "finding msb for %d at rank %d\n", select_, minimal_perfect_hash(h));
  return most_significant_bit(select_, minimal_perfect_hash(h));
}
uint16_t perfect_cuckoo_cache_line::rank(uint16_t bitpos) const {
  uint16_t r = 0;
  uint8_t wordpos = bitpos >> 6; // 6 == log(sizeof(uint64_t)*8) == log(64)
  switch (wordpos) {
    case 5:
      r += rank64(rank_[4]);
    case 4:
      r += rank64(rank_[3]);
    case 3:
      r += rank64(rank_[2]);
    case 2:
      r += rank64(rank_[1]);
    case 1:
      r += rank64(rank_[0]);
  }
  uint8_t inwordpos = bitpos & (sizeof(uint64_t)*8 - 1);
  // fprintf(stderr, "rank for bitpos %d wordpos: %d r inwordpos %d so far: %d\n", bitpos, wordpos, inwordpos, r);
  return r + rank64(rank_[wordpos] & (ones() >> (63-(inwordpos & 63)))); 
}
bool perfect_cuckoo_cache_line::insert(uint32_t h) {
  h = reseed32(h, seed_);
  uint16_t bitpos = h & (sizeof(rank_)*8 - 1);
  uint8_t wordpos = bitpos >> 6; // 6 == log(sizeof(uint64_t)*8) == log(64)
  uint8_t inwordpos = bitpos & (sizeof(uint64_t)*8 - 1);
  // fprintf(stderr, "Inserting %llu at bitpos %d wordpos %d inwordpos %d seed %d\n",
  //                 h, bitpos, wordpos, inwordpos, seed_);
  if (rank_[wordpos] & (1UL << inwordpos)) return false;
  rank_[wordpos] |= (1UL << inwordpos);
  // fprintf(stderr, "new rank for word %d value %llu: %d\n", wordpos, rank_[wordpos], rank64(rank_[wordpos]));
  // select_ |= 1UL << next_empty();
  return true;
}

// From 
// http://people.scs.carleton.ca/~kranakis/Papers/urinal.pdf
static uint64_t const B(int n) {
  uint64_t p = pow(2, abs(log2(n-1))-1);
  return p + floor((n-1)/p)-2*(n-3*p-1) - 1;
}
static uint8_t const A(int n, int i) {
  if (i == 1 || i == n) return 2 + B(n);
  if (i == 2 || i == n-1) return 2 + B(n-1);
  return 3 + B(i) + B(n-i+1);
}

uint8_t perfect_cuckoo_cache_line::next_empty() const {
  return A(sizeof(select_)*8, size());
}
  

}  // namespace cxxmph

#endif
