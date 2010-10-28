#ifndef __CXXMPH_RANDOMLY_SEEDED_HASH__
#define __CXXMPH_RANDOMLY_SEEDED_HASH__

// Helper to create randomly seeded hash functions out of existing hash
// functions that take a seed as a parameter.

#include <cstdlib>

#include "../src/cmph_types.h"
#include "MurmurHash2.h"
#include "stringpiece.h"

namespace cxxmph {

template <class HashFun>
struct RandomlySeededHashFunction { };

class Murmur2StringPiece { };
class Murmur2Pod { };

template <>
struct RandomlySeededHashFunction<Murmur2StringPiece> {
  RandomlySeededHashFunction() : seed(random()) { }
  cmph_uint32 operator()(const StringPiece& key) const {
    return MurmurHash2(key.data(), key.length(), seed);
  }
  cmph_uint32 seed;
};

template<>
struct RandomlySeededHashFunction<Murmur2Pod> {
  RandomlySeededHashFunction() : seed(random()) { }
  template<class Key>
  cmph_uint32 operator()(const Key& key) const {
    return MurmurHash2(&key, sizeof(key), seed);
  }
  cmph_uint32 seed;
};

}  // namespace cxxmph

#endif  // __CXXMPH_RANDOMLY_SEEDED_HASH__
