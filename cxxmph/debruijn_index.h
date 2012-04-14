#include <cstdint>
#include <vector>

#include "mph_bits.h"

namespace cxxmph {

// Maps up to n values into [0;m) [m;n) given m <= n < 32 increasing in the
// first interval and decreasing in the second interval in each insertion.
// The probability of success of the k'th insertion is (k!*(combination(64,
// k))/(64^k). The data structure uses 16 bytes of memory regardless of the
// number of keys.
class debruijn_index {
 public:
  debruijn_index() : n_(16) { clear(); }
  debruijn_index(uint8_t n) : n_(n) { clear(); }
  inline uint8_t index(uint32_t k) const {
    uint8_t b = (k >> seed_) & 63;
    // fprintf(stderr, "Looking for key %u at index %u with pos %u\n", k, b, pos[b]);
    return pos[b];
  }
  bool insert(uint32_t k, uint8_t m) {
    if (m < m_) return false;
    m_ = m;
    uint32_t h = k >> seed_;
    uint8_t b = h & 63;
    // fprintf(stderr, "Inserting %u at bit %d with rank %d\n", k, b, rank64(rank_));
    if (rank_ & (1ULL << b)) { 
      // fprintf(stderr, "Insertion failed: bit collision\n");
      return false;;
    }
    if (rank64(rank_) == 12) {
      // fprintf(stderr, "Insertion failed: full\n");
      return false;;
    }
    rank_ |= 1ULL << b;
    for (uint8_t r = 0; r < rank64(rank_) + 1; ++r) {
      uint8_t slot = select64(rank_, r);
      uint8_t index = r;
      if (r >= m_) index = n_ - (r - m_) - 1;
      pos[slot] = index;
      // fprintf(stderr, "Stored pos %d at slot %d\n", index, slot);
    }
    return true;
  }
  uint8_t size() const { return rank64(rank_); }
  bool reset(const std::vector<uint32_t>& hashes, uint8_t m) {
    uint8_t iterations = 32;
    seed_ = -1;
    while (iterations--) {
      clear();
      m_ = m;
      ++seed_;
      bool success = true;
      for (auto it = hashes.begin(), end = hashes.end(); it != end; ++it) {
        // fprintf(stderr, "calling insert %d\n", m_);
        success &= insert(*it, m_);
        if (!success) break;
      }
      if (success) break;
      // fprintf(stderr, "reset iteration %d for %d keys\n", iterations, hashes.size());
    }
    return iterations >= 0;
  }
  void clear() {
    for (int i = 0; i < 64; ++i) pos[i] = 0;
    m_ = 0; rank_ = 0; seed_ = random() % 27; padding_ = 0;
  }
 private:
  uint64_t rank_;
  uint8_t seed_;
  uint8_t m_;
  uint8_t n_;
  uint8_t padding_;
  uint8_t pos[64];
};
 
}  // namespace cxxmph
