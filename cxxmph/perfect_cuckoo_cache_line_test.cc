#include <cstdlib>
#include <cstdio>
#include <vector>

#include "perfect_cuckoo_cache_line.h"

using cxxmph::perfect_cuckoo_cache_line;
using std::vector;

int main(int argc, char** argv) {
  srandom(2);

  perfect_cuckoo_cache_line pccl;
  fprintf(stderr, "struct size: %d\n", sizeof(perfect_cuckoo_cache_line));
  if (sizeof(perfect_cuckoo_cache_line) > 64) {
    fprintf(stderr, "Too big for a cache line.\n");
    exit(-1);
  }
  if (pccl.size() != 0) {
    fprintf(stderr, "Empty size broken\n");
    exit(-1);
  }

  pccl.clear();
  vector<int> values;
  for (int i = 0; i < 16; ++i) values.push_back(random());
  int iterations = 16;
  while (iterations--) {
    bool success = true;
    pccl.set_seed(random());
    for (int i = 0; i < values.size(); ++i) {
      if (!pccl.insert(values[i])) {
        fprintf(stderr, "Insertion failed\n");
        success = false; break;
      }
    }
    if (success) break;
    pccl.clear();
  }
  if (!(iterations > 0)) exit(-1);
  fprintf(stderr, "Number of iterations left: %d\n", iterations);
  if (!pccl.size() == values.size()) {  
    fprintf(stderr, "Broken size  calculation\n");
    exit(-1);
  }
  vector<bool> found(values.size());
  for (int i = 0; i < values.size(); ++i) {
    auto mph = pccl.minimal_perfect_hash(values[i]);
    if (!(mph < pccl.size())) {
      fprintf(stderr, "Search returned too big: %d size %d\n", mph, pccl.size());
      exit(-1);
    }

    if (found[mph]) {
      fprintf(stderr, "Search broken\n");
      exit(-1);
    }
    auto ph = pccl.perfect_hash(values[i]);
    fprintf(stderr, "key index %u mph %u ph %u\n", i, mph, ph);
    found[mph] = true;
  }
  for (int i = 0; i < values.size(); ++i) {
    if (!found[i]) {
      fprintf(stderr, "Completeness broken.\n");
      exit(-1);
    }
  }
  for (int i = 0; i < values.size(); ++i) {
    auto size = pccl.size();
    auto mph = pccl.minimal_perfect_hash(random());
    if (mph != 255 && !(mph < size)) {
      fprintf(stderr, "Size limit failed for unknown keys: found %d at size %d.\n", mph, size);
      exit(-1);
    }
  }
}
