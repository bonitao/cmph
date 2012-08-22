#ifndef __CXXMPH_MAP_TEST_HELPER_H__
#define __CXXMPH_MAP_TEST_HELPER_H__

#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>
#include <limits>

#include "string_util.h"
#include <check.h>

namespace cxxmph {
template <typename Key>
struct bucketed_key {
  bucketed_key() : key(), bucket(std::numeric_limits<uint64_t>::max()) {}
  bucketed_key(const Key& k, uint64_t b) : key(k), bucket(b) {}
  bool operator==(const bucketed_key<Key>& rhs) const {
    return key == rhs.key;
  }
  Key key;
  uint64_t bucket;
};
template <typename Key>
void tostr(std::ostream* out, const bucketed_key<Key>& k) {
  (*out) << "{" << k.key << "@" << k.bucket << "}";
}
} // namespace cxxmph

namespace std {
template <> template<class Key>
struct hash<cxxmph::bucketed_key<Key>> {
  uint32_t operator()(const cxxmph::bucketed_key<Key>& k) const {
    return hash_(k.key);
  }
 private:
  std::hash<Key> hash_;
};
template <> template<class Key>
struct equal_to<cxxmph::bucketed_key<Key>> {
  bool operator()(const cxxmph::bucketed_key<Key>& a,
                  const cxxmph::bucketed_key<Key>& b) const {
    return equal_(a.key, b.key);
  }
 private:
  std::equal_to<Key> equal_;
};
}  // namespace std

namespace cxxmph {

using namespace std;

template <template<typename...> class map_type>
struct MapTester {
  typedef map_type<bucketed_key<int64_t>, int64_t> test_map;
  typedef map_type<bucketed_key<string>, int64_t> str_test_map;
  static typename test_map::key_type make_key(int64_t k) {
    return typename test_map::key_type(k, k);
  }
  static typename test_map::value_type make_value(int64_t k) {
    return make_pair(make_key(k), k);
  }
  static typename str_test_map::key_type make_str_key(int64_t k) {
    return typename str_test_map::key_type(format("%v", k), k);
  }
  static typename str_test_map::value_type make_str_value(int64_t k) {
    return make_pair(make_str_key(k), k);
  }
  static bool empty_find() {
    test_map m;
    for (int i = 0; i < 1000; ++i) {
      fail_unless(m.find(make_key(i)) == m.end(),
                  "Found key in empty map");
    }
    return true;
  }
  static bool empty_erase() {
    test_map m;
    for (int i = 0; i < 1000; ++i) {
      m.erase(make_key(i));
      if (m.size()) return false;
    }
    return true;
  }
  static bool small_insert() {
    test_map m;
    // Start counting from 1 to not touch default constructed value bugs
    for (int i = 1; i < 12; ++i) m.insert(make_value(i));
    fail_unless(m.size() == 11,
                "Expected map size is 11, got %lu", m.size());
    return m.size() == 11;
  }
  static bool large_insert() {
    test_map m;
    // Start counting from 1 to not touch default constructed value bugs
    int nkeys = 12 * 256 * 32;  // chosen to not timeout on debug builds
    for (int i = 1; i < nkeys; ++i) m.insert(make_value(i));
    return static_cast<int>(m.size()) == nkeys - 1;
  }
  static bool small_search() {
    test_map m;
    // Start counting from 1 to not touch default constructed value bugs
    for (int i = 1; i < 12; ++i) m.insert(make_value(i));
    for (int i = 1; i < 12; ++i) {
      fail_unless(m.find(make_key(i)) != m.end(),
                  "Failed to find key %d just inserted", i);
    }
    return true;
  }
  static bool default_search() {
    test_map m;
    fail_unless(m.find(make_key(0)) == m.end(),
                "Found default value in empty map");
    for (int i = 1; i < 256; ++i) m.insert(make_value(i));
    fail_unless(m.find(make_key(0)) == m.end(),
                "Found 0 value in map holding [1;256)");
    for (int i = 0; i < 256; ++i) m.insert(make_value(i));
    fail_unless(m.find(make_key(0)) != m.end(),
                "Did not find 0 value in map holding [0;256)");
    return true;
  }
  static bool large_search() {
    int nkeys = 10 * 1000;
    test_map m;
    for (int i = 0; i < nkeys; ++i) m.insert(make_value(i));
    for (int i = 0; i < nkeys; ++i) {
      if (m.find(make_key(i)) == m.end()) return false; 
    }
    return true;
  }
  static bool string_search() {
    int nkeys = 10 * 1000;
    str_test_map m;
    for (int i = 0; i < nkeys; ++i) {
      auto v = make_str_value(i);

      fprintf(stderr, "key: %s bucket: %llu value %lld\n", v.first.key.c_str(), v.first.bucket, v.second);
      m.insert(make_str_value(i));
      return true;
    }
    for (int i = 0; i < nkeys; ++i) {
      auto k = make_str_key(i);
      CXXMPH_DEBUGLN("inserting key %v")(k);
      auto it = m.find(k);
      fail_unless(m.find(k) != m.end(),
                  format("cannot find string %v cannot be found", k.key).c_str());
      fail_unless(it->second == i,
                  "value is %d and not expected %d", it->second, i);
    }
    return true;
  }
  static bool rehash_zero() {
    test_map m;
    m.rehash(0);
    return m.size() == 0;
  }
  static bool rehash_size() {
    test_map m;
    int nkeys = 10 * 1000;
    for (int i = 0; i < nkeys; ++i) { m.insert(make_value(i)); }
    m.rehash(nkeys);
    for (int i = 0; i < nkeys; ++i) {
      if (m.find(make_key(i)) == m.end()) return false;
    }
    for (int i = nkeys; i < nkeys * 2; ++i) {
      if (m.find(make_key(i)) != m.end()) return false;
    }
    return true;
  }
  static bool erase_iterator() {
    test_map m;
    int nkeys = 10 * 1000;
    for (int i = 0; i < nkeys; ++i) { m.insert(make_value(i)); }
    for (int i = 0; i < nkeys; ++i) {
       if (m.find(make_key(i)) == m.end()) return false;
    }
    for (int i = nkeys - 1; i >= 0; --i) {
      if (m.find(make_key(i)) == m.end()) return false;
    }
    for (int i = nkeys - 1; i >= 0; --i) {
      fail_unless(m.find(make_key(i)) != m.end(),
                  "after erase %d cannot be found", i);
      fail_unless(m.find(make_key(i))->first.key == i,
                  "after erase key %d cannot be found", i);
    }
    for (int i = nkeys - 1; i >= 0; --i) {
      fail_unless(m.find(make_key(i)) != m.end(),
                  "after erase %d cannot be found", i);
      fail_unless(m.find(make_key(i))->first.key == i,
                  "after erase key %d cannot be found", i);
      if (!(m.find(make_key(i))->first.key == i)) return false;
      m.erase(m.find(make_key(i)));
      if (static_cast<int>(m.size()) != i) return false;
    }
    return true;
  }
  static bool erase_value() {
    test_map m;
    int nkeys = 10 * 1000;
    for (int i = 0; i < nkeys; ++i) { m.insert(make_value(i)); }
    for (int i = nkeys - 1; i >= 0; --i) {
      fail_unless(m.find(make_key(i)) != m.end());
      m.erase(make_key(i));
      if (static_cast<int>(m.size()) != i) return false;
    }
    return true;
  }
};

}  // namespace cxxxmph

#endif // __CXXMPH_MAP_TEST_HELPER_H__
