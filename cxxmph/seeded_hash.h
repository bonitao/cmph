#ifndef __CXXMPH_SEEDED_HASH_H__
#define __CXXMPH_SEEDED_HASH_H__

#include <stdint.h>  // for uint32_t and friends

#include <cstdlib>
#include <unordered_map>  // for std::hash

#include "MurmurHash3.h"
#include "stringpiece.h"

// From murmur, only used naively to extend 32 bits functions to 128 bits.
uint32_t fmix ( uint32_t h );
// Used for a quick and dirty hash function for integers. Probably a bad idea.
uint64_t fmix ( uint64_t h );

namespace cxxmph {

template <class HashFcn>
struct seeded_hash_function {
  template <class Key>
  uint32_t operator()(const Key& k, uint32_t seed) const {
    return HashFcn()(k) ^ seed;
  }
  template <class Key>
  void hash64(const Key& k, uint32_t seed, uint32_t* out) const {
    for (int i = 0; i < 4; ++i) {
      out[i] = HashFcn()(k) ^ seed;
      seed = fmix(seed);
    }
  }
};

struct Murmur3 {
  template<class Key>
  uint32_t operator()(const Key& k) const {
    uint32_t out;
    MurmurHash3_x86_32(reinterpret_cast<const void*>(&k), sizeof(Key), 1 /* seed */, &out);
    return out;
  }
  template <class Key>
  void hash64(const Key& k, uint32_t* out) const {
    MurmurHash3_x64_128(reinterpret_cast<const void*>(&k), sizeof(Key), 1 /* seed */, out);
  }
};

struct Murmur3StringPiece {
  template <class Key>
  uint32_t operator()(const Key& k) const {
    StringPiece s(k);
    uint32_t out;
    MurmurHash3_x86_32(s.data(), s.length(), 1 /* seed */, &out);
    return out;
  }
  template <class Key>
  void hash64(const Key& k, uint32_t* out) const {
    StringPiece s(k);
    MurmurHash3_x64_128(s.data(), s.length(), 1 /* seed */, out);
  }
};

struct Murmur3Fmix64bitsType {
  template <class Key>
  uint32_t operator()(const Key& k) const {
    return fmix(*reinterpret_cast<const uint64_t*>(&k));
  }
  template <class Key>
  void hash64(const Key& k, uint32_t* out) const {
    *reinterpret_cast<uint64_t*>(out) = fmix(k);
    *(out + 2) = fmix(*out);
  }
};

template <>
struct seeded_hash_function<Murmur3> {
  template <class Key>
  uint32_t operator()(const Key& k, uint32_t seed) const {
    uint32_t out;
    MurmurHash3_x86_32(reinterpret_cast<const void*>(&k), sizeof(Key), seed, &out);
    return out;
  }
  template <class Key>
  void hash64(const Key& k, uint32_t seed, uint32_t* out) const {
    MurmurHash3_x64_128(reinterpret_cast<const void*>(&k), sizeof(Key), seed, out);
  }
};

template <>
struct seeded_hash_function<Murmur3StringPiece> {
  template <class Key>
  uint32_t operator()(const Key& k, uint32_t seed) const {
    StringPiece s(k);
    uint32_t out;
    MurmurHash3_x86_32(s.data(), s.length(), seed, &out);
    return out;
  }
  template <class Key>
  void hash64(const Key& k, uint32_t seed, uint32_t* out) const {
    StringPiece s(k);
    MurmurHash3_x64_128(s.data(), s.length(), seed, out);
  }
};

template <>
struct seeded_hash_function<Murmur3Fmix64bitsType> {
  template <class Key>
  uint32_t operator()(const Key& k, uint32_t seed) const {
    return fmix(k + seed);
  }
  template <class Key>
  void hash64(const Key& k, uint32_t seed, uint32_t* out) const {
    *reinterpret_cast<uint64_t*>(out) = fmix(k ^ seed);
    *(out + 2) = fmix(*out);
  }
};


template <class HashFcn> struct seeded_hash
{ typedef seeded_hash_function<HashFcn> hash_function; };
// Use Murmur3 instead for all types defined in std::hash, plus
// std::string which is commonly extended.
template <> struct seeded_hash<std::hash<char*> >
{ typedef seeded_hash_function<Murmur3StringPiece> hash_function; };
template <> struct seeded_hash<std::hash<const char*> >
{ typedef seeded_hash_function<Murmur3StringPiece> hash_function; };
template <> struct seeded_hash<std::hash<std::string> >
{ typedef seeded_hash_function<Murmur3StringPiece> hash_function; };
template <> struct seeded_hash<std::hash<cxxmph::StringPiece> >
{ typedef seeded_hash_function<Murmur3StringPiece> hash_function; };

template <> struct seeded_hash<std::hash<char> >
{ typedef seeded_hash_function<Murmur3> hash_function; };
template <> struct seeded_hash<std::hash<unsigned char> >
{ typedef seeded_hash_function<Murmur3> hash_function; };
template <> struct seeded_hash<std::hash<short> >
{ typedef seeded_hash_function<Murmur3> hash_function; };
template <> struct seeded_hash<std::hash<unsigned short> >
{ typedef seeded_hash_function<Murmur3> hash_function; };
template <> struct seeded_hash<std::hash<int> >
{ typedef seeded_hash_function<Murmur3> hash_function; };
template <> struct seeded_hash<std::hash<unsigned int> >
{ typedef seeded_hash_function<Murmur3> hash_function; };
template <> struct seeded_hash<std::hash<long> >
{ typedef seeded_hash_function<Murmur3> hash_function; };
template <> struct seeded_hash<std::hash<unsigned long> >
{ typedef seeded_hash_function<Murmur3> hash_function; };
template <> struct seeded_hash<std::hash<long long> >
{ typedef seeded_hash_function<Murmur3> hash_function; };
template <> struct seeded_hash<std::hash<unsigned long long> >
{ typedef seeded_hash_function<Murmur3> hash_function; };

}  // namespace cxxmph

#endif  // __CXXMPH_SEEDED_HASH_H__
