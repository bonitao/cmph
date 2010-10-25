#include <cassert>
#include <vector>

#include "mphtable.h"

using std::vector;
using cxxmph::MPHTable;

int main(int argc, char** argv) {
  vector<int> keys;
  keys.push_back(10);
  keys.push_back(4);
  keys.push_back(3);

  MPHTable<int> mphtable;
  assert(mphtable.Reset(keys.begin(), keys.end()));
  vector<int> ids;
  for (int i = 0; i < keys.size(); ++i) ids.push_back(mphtable.index(keys[i]));
  sort(ids.begin(), ids.end());
  for (int i = 0; i < ids.size(); ++i) assert(ids[i] == i);
}
  
