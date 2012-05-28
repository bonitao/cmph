#ifndef __CXXMPH_MAP_TEST_HELPER_H__
#define __CXXMPH_MAP_TEST_HELPER_H__

#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <unordered_map>

namespace cxxmph {

using namespace std;

// template <template <class K, class V> class unordered_map> >
class MapTester {
 public:
  MapTester();
  ~MapTester();
  bool Run(string* errors) const {
    string e;
    if (!small_insert()) e += "small insert failed\n"; 
    if (!large_insert()) e += "large insert failed\n"; 
    if (!small_search()) e += "small search failed\n"; 
    if (!default_search()) e += "default search failed\n"; 
    if (!large_search()) e += "large search failed\n"; 
    if (errors) *errors = e;
    return !e.empty();
  }
  static bool small_insert() {
    unordered_map<int64_t, int64_t> m;
    // Start counting from 1 to not touch default constructed value bugs
    for (int i = 1; i < 12; ++i) m.insert(make_pair(i, i));
    return m.size() == 11;
  }
  static bool large_insert() {
    unordered_map<int64_t, int64_t> m;
    // Start counting from 1 to not touch default constructed value bugs
    for (int i = 1; i < 12 * 256 * 256; ++i) m.insert(make_pair(i, i));
    return m.size() == 12 * 256 * 256 - 1;
  }
  static bool small_search() {
    unordered_map<int64_t, int64_t> m;
    // Start counting from 1 to not touch default constructed value bugs
    for (int i = 1; i < 12; ++i) m.insert(make_pair(i, i));
    for (int i = 1; i < 12; ++i) if (m.find(i) == m.end()) return false; 
    return true;
  }
  static bool default_search() {
    unordered_map<int64_t, int64_t> m;
    if (m.find(0) != m.end()) return false;
    for (int i = 1; i < 256; ++i) m.insert(make_pair(i, i));
    if (m.find(0) != m.end()) return false;
    for (int i = 0; i < 256; ++i) m.insert(make_pair(i, i));
    if (m.find(0) == m.end()) return false;
    return true;
  }
  static bool large_search() {
    int nkeys = 10 * 1000;
    unordered_map<int64_t, int64_t> m;
    for (int i = 0; i < nkeys; ++i) m.insert(make_pair(i, i));
    for (int i = 0; i < nkeys; ++i) if (m.find(i) == m.end()) return false; 
    return true;
  }
  static bool string_search() {
    int nkeys = 10 * 1000;
    vector<string> keys;
    for (int i = 0; i < nkeys; ++i) {
      char buf[128];
      snprintf(buf, sizeof(buf), "%lu", i);
      keys.push_back(buf);
    }
    unordered_map<string, int64_t> m;
    for (int i = 0; i < nkeys; ++i) m.insert(make_pair(keys[i], i));
    for (int i = 0; i < nkeys; ++i) {
      auto it = m.find(keys[i]);
      if (it == m.end()) return false;
      if (it->second != i) return false;
    }
    return true;
  }
};

}  // namespace cxxxmph

#endif // __CXXMPH_MAP_TEST_HELPER_H__
