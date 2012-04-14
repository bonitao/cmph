#include <cstdio>
#include <vector>
#include "debruijn_index.h" 
#include "mph_bits.h"

using namespace std;
using cxxmph::debruijn_index;
using cxxmph::reseed32;

int main(int argc, char** argv) {
  srandom(8);
  uint32_t seed = random();
  debruijn_index ri(16);
  if (!ri.insert(reseed32(8, seed), 4)) {
    fprintf(stderr, "Insertion failed\n");
  }
  if (ri.size() != 1) {
    fprintf(stderr, "Wrong size: %d\n", ri.size());
    exit(-1);
  }
  if (ri.index(reseed32(8, seed)) != 0) {
    fprintf(stderr, "Found key at wrong index %d\n", ri.index(3));
    exit(-1);
  }
  for (int i = 0; i < 8; ++i) {
    if (!ri.insert(reseed32(i, seed), 4)) {
      fprintf(stderr, "The %d th insertion failed\n", i);
      exit(-1);
    }
  }
  vector<bool> p(9);
  for (int i = 0; i < 9; ++i) {
    if (p[ri.index(reseed32(i, seed))]) {
      fprintf(stderr, "The %d key collided at %d\n", i, ri.index(i));
      exit(-1);
    }
    p[ri.index(reseed32(i, seed))] = true;
  }
  for (int i = 0; i < 4; ++i) {
    if (!p[i]) {
      fprintf(stderr, "Missed key %d\n", i);
      exit(-1);
    }
  }
}
