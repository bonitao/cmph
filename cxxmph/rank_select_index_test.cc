#include <cstdio>
#include <vector>

#include "rank_select_index.h"

using namespace std;
using cxxmph::rank_select_index;

int main(int argc, char** argv) {
  rank_select_index ri(16);
  if (!ri.insert(8, 4)) {
    fprintf(stderr, "Insertion failed\n");
  }
  if (ri.size() != 1) {
    fprintf(stderr, "Wrong size: %d\n", ri.size());
    exit(-1);
  }
  if (ri.index(8) != 0) {
    fprintf(stderr, "Found key at wrong index %d\n", ri.index(3));
    exit(-1);
  }
  for (int i = 0; i < 8; ++i) {
    if (!ri.insert(i, 4)) {
      fprintf(stderr, "The %d th insertion failed\n", i);
      exit(-1);
    }
  }
  vector<bool> p(9);
  for (int i = 0; i < 9; ++i) {
    if (p[ri.index(i)]) {
      fprintf(stderr, "The %d key collided at %d\n", i, ri.index(i));
      exit(-1);
    }
    p[ri.index(i)] = true;
  }
  for (int i = 0; i < 4; ++i) {
    if (!p[i]) {
      fprintf(stderr, "Missed key %d\n", i);
      exit(-1);
    }
  }
}
