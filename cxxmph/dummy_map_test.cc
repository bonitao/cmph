#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "dummy_map.h"

using std::cerr;
using std::endl;
using std::make_pair;
using std::pair;
using std::string;
using cxxmph::dummy_map;

int main(int argc, char** argv) {
  dummy_map<pair<int64_t, uint32_t>, int64_t> b;
  int32_t num_keys = 19;
  for (int i = 0; i < num_keys; ++i) {
    cerr << "Calling insert " << endl; 
    b.insert(make_pair(make_pair(i, i), i));
  }
  cerr << "Rehashing" << endl;
  b.rehash(b.size());
  for (int i = 0; i < 100; ++i) {
    auto k = i % num_keys;
    auto it = b.find(make_pair(k, k));
    if (it == b.end()) {
      std::cerr << "Failed to find " << i << std::endl;
      exit(-1);
    }
    if (it->first.first != it->second || it->first.first != i % num_keys) {
      std::cerr << "Found " << it->first.first << " looking for " << i << std::endl;
      exit(-1);
    }
  }
  cerr << "Ended with success" << endl;

  dummy_map<pair<string, uint32_t>, int> h;
  h.insert(make_pair(make_pair("-1", 102), -1));
  dummy_map<pair<string, uint32_t>, int>::const_iterator it;
  // for (auto it = h.begin(); it != h.end(); ++it) {
  //  if (it->second != -1) exit(-1);
  //}
  int32_t num_valid = 100;
  for (int i = 0; i < num_valid; ++i) {
     char buf[10];    
     snprintf(buf, 10, "%d", i);
     h.insert(make_pair(make_pair(buf, i), i));
  }
  for (int j = 0; j < 100; ++j) {
    for (int i = 1000; i > 0; --i) {
       char buf[10];    
       snprintf(buf, 10, "%d", i - 1);
       auto it = h.find(make_pair(buf, i - 1));
       if (i < num_valid && it->second != i - 1) exit(-1);
    }
  }
  for (int j = 0; j < 100; ++j) {
    for (int i = 1000; i > 0; --i) {
       char buf[10];    
       int key = i*100 - 1;
       snprintf(buf, 10, "%d", key);
       auto it = h.find(make_pair(buf, key));
       if (key < num_valid && it->second != key) exit(-1);
    }
  }
  fprintf(stderr, "Size is %lu\n", h.size());
  for (int i = 0; i < num_valid; ++i) {
     char buf[10];    
     snprintf(buf, 10, "%ld", i);
     h.erase(make_pair(buf, i));
  }
  if (h.size() != 1) {
    fprintf(stderr, "Erase failed: size is %lu\n", h.size());
    exit(-1);
  }
}
