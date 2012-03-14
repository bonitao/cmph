#include <cstdio>
#include <cstdlib>

#include "mph_bits.h"

using cxxmph::dynamic_2bitset;
int main(int argc, char** argv) {
  int size = 256;
  dynamic_2bitset bits(size, true /* fill with ones */);
  for (int i = 0; i < size; ++i) {
    if (bits[i] != 3)  {
      fprintf(stderr, "wrong bits %d at %d expected %d\n", bits[i], i, 3);
      exit(-1);
    }
  }
  for (int i = 0; i < size; ++i) bits.set(i, 0);
  for (int i = 0; i < size; ++i) {
    if (bits[i] != 0)  {
      fprintf(stderr, "wrong bits %d at %d expected %d\n", bits[i], i, 0);
      exit(-1);
    }
  }
  for (int i = 0; i < size; ++i) bits.set(i, i % 4);
  for (int i = 0; i < size; ++i) {
    if (bits[i] != i % 4) {
      fprintf(stderr, "wrong bits %d at %d expected %d\n", bits[i], i, i % 4);
      exit(-1);
    }
  }
}
  
  
