#ifndef __CXXMPH_RANDOMLY_SEEDED_HASH__
#define __CXXMPH_RANDOMLY_SEEDED_HASH__

// Helper to create randomly seeded hash functions out of existing hash
// functions that take a seed as a parameter.

#include <cstdlib>

#include "../src/cmph_types.h"
#include "MurmurHash2.h"

namespace cxxmph {

struct RandomlySeededMurmur2 {
  RandomlySeededHashFunction() : seed(random()) { }
  cmph_uint32 operator()(const StringPiece& key) {
    return MurmurHash2(key.data(), key.length(), seed);
  }
  cmph_uint32 seed;
};

}  // namespace cxxmph

#endif  // __CXXMPH_RANDOMLY_SEEDED_HASH__
