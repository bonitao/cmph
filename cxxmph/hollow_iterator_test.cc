#include <cstdlib>
#include <cstdio>
#include <vector>

#include "hollow_iterator.h"

using std::vector;
using cxxmph::hollow_iterator;
using cxxmph::hollow_const_iterator;

int main(int argc, char** argv) {
  vector<int> v;
  vector<bool> p;
  for (int i = 0; i < 100; ++i) {
    v.push_back(i);
    p.push_back(i % 2 == 0);
  }
  auto begin = hollow_iterator<vector<int>>(&v, &p, v.begin());
  auto end = hollow_iterator<vector<int>>(&v, &p, v.end());
  for (auto it = begin; it != end; ++it) {
    if (((*it) % 2) != 0) exit(-1);
  }
  hollow_const_iterator<vector<int>> const_begin(begin);
  hollow_const_iterator<vector<int>> const_end(end);
  for (auto it = const_begin; it != const_end; ++it) {
    if (((*it) % 2) != 0) exit(-1);
  }
  vector<int>::iterator vit1 = v.begin();
  vector<int>::const_iterator vit2 = v.begin();
  if (vit1 != vit2) exit(-1);
  auto it1 = hollow_iterator<vector<int>>(&v, &p, v.begin());
  auto it2 = hollow_const_iterator<vector<int>>(&v, &p, v.begin());
  if (it1 != it2) exit(-1);
}

