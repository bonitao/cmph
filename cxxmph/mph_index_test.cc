#include <algorithm>
#include <cassert>
#include <iostream>
#include <string>
#include <vector>

#include "mph_index.h"

using std::cerr;
using std::endl;
using std::string;
using std::vector;
using namespace cxxmph;

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

  SimpleMPHIndex<string> mph_index;
  if (!mph_index.Reset(keys.begin(), keys.end(), keys.size())) { exit(-1); }
  vector<int> ids;
  for (vector<int>::size_type i = 0; i < keys.size(); ++i) {
     ids.push_back(mph_index.index(keys[i]));
     cerr << " " << *(ids.end() - 1);
  }
  cerr << endl;
  sort(ids.begin(), ids.end());
  for (vector<int>::size_type i = 0; i < ids.size(); ++i) assert(ids[i] == static_cast<vector<int>::value_type>(i));

  vector<int> k2;
  for (int i = 0; i < 512;  ++i) { k2.push_back(i); }
  SimpleMPHIndex<int> k2_index;
  if (!k2_index.Reset(k2.begin(), k2.end(), k2.size())) { exit(-1); }
}
