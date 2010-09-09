#include "cmph_hash_map.h"

#include <iostream>

int main(int argc, char** argv) {
  cmph_hash_map<int, int> h;
  h.insert(std::make_pair(-1,-1));
  for (cmph_hash_map<int, int>::const_iterator it = h.begin(); it != h.end(); ++it) {
    std::cout << it->first << " -> " << it->second << std::endl;
  }
  std::cout << "Search -1 gives " << h.find(-1)->second << std::endl;
  for (int i = 0; i < 1000; ++i) h.insert(std::make_pair(i, i));
  for (int j = 0; j < 1000; ++j) {
    for (int i = 1000; i > 0; --i) {
      h.find(i - 1);
      // std::cout << "Search " << i - 1 << " gives " << h.find(i - 1)->second << std::endl;
    }
  }
}
