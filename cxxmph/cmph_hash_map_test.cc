#include "cmph_hash_map.h"

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

using std::make_pair;
using std::string;
using cxxmph::cmph_hash_map;

int main(int argc, char** argv) {
  cmph_hash_map<int64_t, int64_t> b;
  for (int i = 0; i < 257; ++i) {
    b.insert(make_pair(i, i));
  }
  /*
  cmph_hash_map<string, int> h;
  h.insert(std::make_pair("-1",-1));
  cmph_hash_map<string, int>::const_iterator it;
  for (it = h.begin(); it != h.end(); ++it) {
    std::cerr << it->first << " -> " << it->second << std::endl;
  }
  std::cerr << "Search -1 gives " << h.find("-1")->second << std::endl;
  for (int i = 0; i < 100; ++i) {
     char buf[10];    
     snprintf(buf, 10, "%d", i);
     h.insert(std::make_pair(buf, i));
  }
  for (int j = 0; j < 100; ++j) {
    for (int i = 1000; i > 0; --i) {
       char buf[10];    
       snprintf(buf, 10, "%d", i - 1);
       h.find(buf);
       std::cerr << "Search " << i - 1 << " gives " << h.find(buf)->second << std::endl;
    }
  }
  for (int j = 0; j < 100; ++j) {
    for (int i = 1000; i > 0; --i) {
       char buf[10];    
       snprintf(buf, 10, "%d", i*100 - 1);
       h.find(buf);
       std::cerr << "Search " << i*100 - 1 << " gives " << h.find(buf)->second << std::endl;
    }
  }
  */

}
