#include "cmph_hash_map.h"

#include <cstdlib>
#include <iostream>
#include <string>

using std::string;
using cxxmph::cmph_hash_map;

int main(int argc, char** argv) {
  cmph_hash_map<string, int> h;
  h.insert(std::make_pair("-1",-1));
  cmph_hash_map<string, int>::const_iterator it;
  for (it = h.begin(); it != h.end(); ++it) {
    std::cout << it->first << " -> " << it->second << std::endl;
  }
  std::cout << "Search -1 gives " << h.find("-1")->second << std::endl;
  for (int i = 0; i < 1000; ++i) {
     char buf[10];    
     snprintf(buf, 10, "%d", i);
     h.insert(std::make_pair(buf, i));
  }
  for (int j = 0; j < 1000; ++j) {
    for (int i = 1000; i > 0; --i) {
       char buf[10];    
       snprintf(buf, 10, "%d", i - 1);
       h.find(buf);
       // std::cout << "Search " << i - 1 << " gives " << h.find(i - 1)->second << std::endl;
    }
  }
}
