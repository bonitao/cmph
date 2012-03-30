#ifndef __CXXMPH_PERFECT_CUCKOO_CACHE_LINE_H__
#define __CXXMPH_PERFECT_CUCKOO_CACHE_LINE__

#include <cmath>

#include "rank9.h"
#include "mph_bits.h"

namespace cxxmph {

__inline static int count( const uint64_t x ) {
		register uint64_t byte_sums = x - ( ( x & 0xa * ONES_STEP_4 ) >> 1 );
		byte_sums = ( byte_sums & 3 * ONES_STEP_4 ) + ( ( byte_sums >> 2 ) & 3 * ONES_STEP_4 );
		byte_sums = ( byte_sums + ( byte_sums >> 4 ) ) & 0x0f * ONES_STEP_8;
		return byte_sums * ONES_STEP_8 >> 56;
	}

__inline uint32_t static_rank( const uint64_t k, const uint64_t* sbits, const uint64_t* scounts ) {
	const uint64_t word = k >> 6;
	const uint64_t block = (word >> 2) & ~1;
	const int offset = (word & 7) - 1;
	return scounts[ block ] + ( scounts[ block + 1 ] >> ( offset + ( offset >> sizeof offset * 8 - 4 & 0x8 ) ) * 9 & 0x1FF ) + count( sbits[ word ] & ( ( 1ULL << k % 64 ) - 1 ) );
}



class perfect_cuckoo_cache_line {
 public:
  perfect_cuckoo_cache_line() : seed_(0) { clear(); }
  void set_seed(uint32_t seed) { seed_ = seed; }
  const uint32_t seed() const { return seed_; }
  
  uint8_t size() const { return rank((sizeof(rank_)*8) - 1); }
  static uint8_t max_capacity() { return sizeof(uint64_t)*8; }
  static uint8_t good_capacity() { return floor(sqrt(sizeof(rank_)*8)); }

  uint8_t perfect_hash(uint32_t h) const;
  uint8_t minimal_perfect_hash(uint32_t h) const;
  uint8_t minimal_perfect_hash2(uint32_t h) const;
  bool insert(uint32_t h);

  void clear() {
    memset(rank_, 0, sizeof(rank_));
    memset(count_, 0, sizeof(count_));
  }
 private:
  uint16_t rank(uint16_t pos) const;
  uint8_t next_empty() const;
  uint64_t rank_[4];  // 52
  uint64_t count_[2];  // 20
  uint32_t seed_;  // 4
};

uint8_t perfect_cuckoo_cache_line::minimal_perfect_hash(uint32_t h) const {
  h = reseed32(h, seed_);
  uint16_t bitpos = h & (sizeof(rank_)*8 - 1);
  // uint8_t wordpos = bitpos >> 6; // 6 == log(sizeof(uint64_t)*8) == log(64)
  // uint8_t inwordpos = bitpos & (sizeof(uint64_t)*8 - 1);
  // fprintf(stderr, "Searching %llu at bitpos %d wordpos %d inwordpos %d\n",
  //                 h, bitpos, wordpos, inwordpos);
  return static_rank(bitpos+1, rank_, count_) - 1;
  return rank(bitpos) - 1; 
}
uint8_t perfect_cuckoo_cache_line::minimal_perfect_hash2(uint32_t h) const {
  h = reseed32(h, seed_);
  uint16_t bitpos = h & (sizeof(rank_)*8 - 1);
  // uint8_t wordpos = bitpos >> 6; // 6 == log(sizeof(uint64_t)*8) == log(64)
  // uint8_t inwordpos = bitpos & (sizeof(uint64_t)*8 - 1);
  // fprintf(stderr, "Searching %llu at bitpos %d wordpos %d inwordpos %d\n",
  //                 h, bitpos, wordpos, inwordpos);
  return static_rank(bitpos+1, rank_, count_) - 1;
  return rank9::static_rank(bitpos+1, rank_, count_) - 1;
  return rank(bitpos) - 1; 
}



uint8_t perfect_cuckoo_cache_line::perfect_hash(uint32_t h) const {
  // fprintf(stderr, "finding msb for %d at rank %d\n", select_, minimal_perfect_hash(h));
  // return most_significant_bit(select_, minimal_perfect_hash(h));
  return 0;
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
  //                h, bitpos, wordpos, inwordpos, seed_);
  if (rank_[wordpos] & (1ULL << inwordpos)) return false;
  rank_[wordpos] |= (1ULL << inwordpos);
  rank9 r(rank_, sizeof(rank_)*8) ;
  assert(sizeof(count_) == r.bit_count()/8);
  memcpy(count_, r.scounts(), sizeof(count_));
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
  return 0; //A(sizeof(select_)*8, size());
}
  

}  // namespace cxxmph

#endif
