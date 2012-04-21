#include <cstdint>
#include <vector>

#include "mph_bits.h"

namespace cxxmph {

// Maps up to n values into [0;m) [m;n) given m <= n < 32 increasing in the
// first interval and decreasing in the second interval in each insertion.
// The probability of success of the k'th insertion is (k!*(combination(64,
// k))/(64^k). The data structure uses 16 bytes of memory regardless of the
// number of keys.
class rank_select_index {
 public:
  static uint8_t begin() { return 0; }
  static uint8_t end() { return 32; }
  static uint8_t capacity() { return 16; }

  rank_select_index() { clear(); }
  inline uint8_t index(uint32_t k) const {
    uint32_t h = reseed32(k, seed_);
    uint8_t b = h & 63;
    uint8_t r = rank64(rank_ & ((1ULL << b)-1));
    uint8_t s = select64(select_, r) & 63;
    // fprintf(stderr, "Found key %d at rank %d slot %d\n", k, r, s);
    return s;
  }
  uint8_t size() const { return rank64(rank_); }
  bool insert(uint32_t k, uint8_t m) {
    if (m < m_) return false;
    // fprintf(stderr, "insert m %d\n", m);
    m_ = m;
    uint32_t h = reseed32(k, seed_);
    uint8_t b = h & 63;
    if (rank_ & (1ULL << b)) return false;;
    rank_ |= 1ULL << b;
    select_ = 0;
    // fprintf(stderr, "Inserting key %d at bit %d rank %d with %d keys\n", k, b, rank64(rank_ & ((1ULL << b)-1)), rank64(rank_));
    for (uint8_t r = 0; r < rank64(rank_); ++r) {
      uint8_t slot = r;
      if (r >= m_) slot = n_ - (r - m_) - 1;
      // fprintf(stderr, "Slot for key of rank %d at m %d is %d\n", r, m_, slot);
      select_ |= (1ULL << slot);
    }
    return true;
  }
  bool reset(const std::vector<uint32_t>& keys, std::vector<uint32_t>& slots,
             std::vector<pair<uint32_t, uint32_t>>* out) {
    // fprintf(stderr, "Resetting for %d keys and m %d\n", hashes.size(), m_);
    uint8_t iterations = 255;
    while (iterations--) {
      clear();
      m_ = m;
      bool success = true;
      for (auto it = hashes.begin(), end = hashes.end(); it != end; ++it) {
        // fprintf(stderr, "calling insert %d\n", m_);
        success &= insert(*it, m_);
      }
      if (success) break;
    }
    return iterations >= 0;
  }
  void clear() { m_ = 0; rank_ = 0; seed_ = random(); select_ = 0; padding_ = 0; }
 private:
  uint64_t rank_;
  uint32_t select_; 
  uint8_t seed_;
  uint8_t padding_[3];
};
 
}  // namespace cxxmph
