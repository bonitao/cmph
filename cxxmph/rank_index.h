#ifndef __CXXMPH_RANK_INDEX_H__
#define __CXXMPH_RANK_INDEX_H__

#include <cmath>
#include <cstdlib>
#include <memory>

#include "mph_bits.h"
#include "seeded_hash.h"
#include "sux-0.7/rank9.h"

namespace cxxmph {

// Number of bytes for rank structure, a 32 bits seed and a 32 bits offset
// (((numbits + 64 * 8 - 1 ) / ( 64 * 8 ) ) * 2 + 1 + 1)*8
// For 64 bytes cache line, this is 1536
//
// bytes/8 = (((numbits + 64 * 8 - 1 ) / ( 64 * 8 ) ) * 2 + 1 + 1)
// bytes/8 - 2 = (((numbits + 64 * 8 - 1 ) / ( 64 * 8 ) ) * 2)
// (bytes/8 - 2)/2 = ((numbits + 64 * 8 - 1 ) / ( 64 * 8 ) )
// ((bytes/8 - 2)/2)*(64*8) = ((numbits + 64 * 8 - 1 )
// ((bytes/8 - 2)/2)*(64*8) + 1 = numbits + 64 * 8
// ((bytes/8 - 2)/2)*(64*8) + 1 - 64*8 = numbits

class RankIndex {
 public:
  RankIndex() : bits_(NULL) {}

  template <class SeededHashFcn, class ForwardIterator>
  bool Reset(ForwardIterator begin, ForwardIterator end, uint32_t size);
  uint32_t size() const { return m_; }
  template <class SeededHashFcn, class Key>  // must agree with Reset
  // Get a unique identifier for k, in the range [0;size()). If x wasn't part
  // of the input in the last Reset call, returns a random value.
  uint32_t index(const Key& x) const;
  void clear() { m_ = 0; num_bits_ = 0; delete [] bits_; bits_ = NULL; rank9_.reset(); }

 private:
  std::unique_ptr<rank9> rank9_;;
  uint64_t* bits_;
  uint64_t num_bits_;
  uint64_t m_;
  std::vector<uint32_t> seed_;
  uint32_t block_seed_;
};

// Template method needs to go in the header file.
template <class SeededHashFcn, class ForwardIterator>
bool RankIndex::Reset(
    ForwardIterator begin, ForwardIterator end, uint32_t size) {
  if (end == begin) {
    clear();
    return true;
  }
  m_ = size;
  std::cerr << "size " << m_ << std::endl;
  uint32_t bits_per_block = 2048;
  uint64_t blocksize = bits_per_block/(sizeof(uint64_t)*8)
  uint32_t keys_per_block = floor(sqrt(2048));
  uint64_t num_blocks = nextpoweroftwo(ceil(m_*1.0/keys_per_block));
  std::cerr << "num blocks " << num_blocks << std::endl;
  num_bits_ = num_blocks*bits_per_block;
  std::cerr << "num bits " << num_bits_ << std::endl;
  int iterations = 1000;
  while (iterations--) {
    bool success = true;
    std::cerr << "iteration " << iterations << std::endl;
    delete [] bits_;
    bits_ = new uint64_t[num_blocks];
    memset(bits_, 0, num_blocks*sizeof(uint64_t));
    vector<uint32_t>(num_blocks).swap(seed);
    vector<vector<uint32_t>> hashed_keys(num_blocks);
    for (auto it = begin, it != end; ++it) {
      uint32_t h = SeededHashFcn()(*it, block_seed);
      hashed_keys[h & (num_blocks - 1)].push_back(h);
    }
    // Idea is right, math is all wrong
    while (iterations-- && !success) for (int i = 0; i < hashed_keys.size(); ++i) {
      success = true;
      vector<bool> collision_(bits_per_block);
      seed_[i] = random();
      for (uint j = 0; j < blocksize; ++j) {
        bits_[i*blocksize + j] = 0;
      }
      for (uint j = 0; j < hashed_keys[i].size(); ++j) {
        uint32_t bit = (hashed_keys[i][j] ^ seed_[i]) & (bits_per_block-1);
        if (collision[bit]) {
          success = false;
          break;
        } else collision[bit] = true;
        bits_[i*blocksize + bit % blocksize] |=
            (1LL << ((bit % blocksize) % blocks)));
      }
    }
  }
         
      
      uint32_t block = i / 8;
      if (i & 8 == 0) seed_[block] = random();
      while (iterations--) for (auto jit = it, int j = 0; jit != end() && j < 8; ++jit, ++j) { 
        uint64_t bit = h & (num_bits_ - 1);
      // std::cerr << "key " << *it << " bit " << bit << " block " << bit/64 << " offset " << bit % 64 << std::endl;
      if (bits_[bit / 64] & (1LL << (bit % 64))) {
        // std::cerr << "collision " << std::endl;
        success = false;
        break;
      }
      bits_[bit / 64] |= (1LL << (bit % 64));
    }
    if (success) {
      rank9_.reset(new rank9(bits_, num_bits_));
      break;
    }
  }
  return iterations != 0;
}

template <class SeededHashFcn, class Key>
uint32_t RankIndex::index(const Key& key) const {
  uint32_t h = SeededHashFcn()(key, seed_);
  uint64_t bitpos = h & (num_bits_ - 1);
  assert(bitpos < num_bits_);
  // std::cerr << " bit pos " << bitpos << std::endl;
  return rank9_->rank(bitpos);
}

// Simple wrapper around RankIndex to simplify calling code. Please refer to the
// RankIndex class for documentation.
template <class Key, class HashFcn = typename seeded_hash<std::hash<Key>>::hash_function>
class SimpleRankIndex : public RankIndex {
 public:
  template <class ForwardIterator>
  bool Reset(ForwardIterator begin, ForwardIterator end, uint32_t size) {
    return RankIndex::Reset<HashFcn>(begin, end, size);
  }
  uint32_t index(const Key& key) const { return RankIndex::index<HashFcn>(key); }
  uint32_t size() const { return RankIndex::size(); }
};

}  // namespace cxxmph

#endif
