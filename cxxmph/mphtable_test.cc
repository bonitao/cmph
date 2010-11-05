#include <algorithm>
#include <cassert>
#include <string>
#include <vector>

#include "mphtable.h"

using std::string;
using std::vector;
using cxxmph::SimpleMPHTable;

int main(int argc, char** argv) {

  srand(1);
  vector<string> keys;
  keys.push_back("davi");
  keys.push_back("paulo");
  keys.push_back("joao");
  keys.push_back("maria");
  keys.push_back("bruno");
  keys.push_back("paula");
  keys.push_back("diego");
  keys.push_back("diogo");
  keys.push_back("algume");

  SimpleMPHTable<string> mphtable;
  assert(mphtable.Reset(keys.begin(), keys.end()));
  vector<int> ids;
  for (vector<int>::size_type i = 0; i < keys.size(); ++i) {
     ids.push_back(mphtable.index(keys[i]));
     cerr << " " << *(ids.end() - 1);
  }
  cerr << endl;
  sort(ids.begin(), ids.end());
  for (vector<int>::size_type i = 0; i < ids.size(); ++i) assert(ids[i] == static_cast<vector<int>::value_type>(i));
}
  
