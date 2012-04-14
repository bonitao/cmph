#include <string>
#include <utility>

#include "perfect_cuckoo.h"

using cxxmph::perfect_cuckoo_map;
using std::string;
using std::cerr;
using std::endl;
using std::make_pair;

int main(int argc, char** argv) {
  perfect_cuckoo_map<int64_t, int64_t> b;
  int32_t num_keys = 10*1;
  for (int i = 0; i < num_keys; ++i) {
    if (!b.insert(make_pair(i, i)).second) {
      std::cerr << "Insertion of unknown key failed" << std::endl;
      exit(-1);
    }
  }
  // b.rehash(b.size());
  for (int i = 0; i < 100; ++i) {
    auto it = b.find(i % num_keys);
    if (it == b.end()) {
      std::cerr << "Failed to find " << i << std::endl;
      exit(-1);
    }
    if (it->first != it->second || it->first != i % num_keys) {
      std::cerr << "Found " << it->first << " looking for " << i << std::endl;
      exit(-1);
    }
  }

  perfect_cuckoo_map<string, int> h;
  h.insert(std::make_pair("-1",-1));
  perfect_cuckoo_map<string, int>::const_iterator it;
  for (it = h.begin(); it != h.end(); ++it) {
    if (it->second != -1) {
      cerr << "Failed to find value in single key map" << endl;
      exit(-1);
    }
  }
  h.find("-1")->second = -2;
  h["-1"] = -3;
  if (h["-1"] != -3) {
    cerr << "operator[] failed" << endl;
    exit(-1);
  }
  int32_t num_valid = 100;
  for (int i = 0; i < num_valid; ++i) {
     char buf[10];    
     snprintf(buf, 10, "%d", i);
     h.insert(std::make_pair(buf, i));
     auto it = h.find(buf);
     if (it->second != i) {
       cerr << "Failed search after insertion " << i << " got " << it->second << endl;
       exit(-1);
     }
  }
  for (int j = 0; j < 100; ++j) {
    for (int i = 1000; i > 0; --i) {
       char buf[10];    
       snprintf(buf, 10, "%d", i - 1);
       auto it = h.find(buf);
       if (i < num_valid && it->second != i - 1) {
         cerr << "Failed looking for " << (i-1) << " got " << it->second << endl;
         exit(-1);
       }
    }
  }
  for (int j = 0; j < 100; ++j) {
    for (int i = 1000; i > 0; --i) {
       char buf[10];    
       int key = i*100 - 1;
       snprintf(buf, 10, "%d", key);
       auto it = h.find(buf);
       if (key < num_valid && it->second != key) exit(-1);
    }
  }
}
