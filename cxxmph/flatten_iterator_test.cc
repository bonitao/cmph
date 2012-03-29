#include <cstdlib>
#include <cstdio>
#include <string>
#include <vector>

#include "flatten_iterator.h"

using namespace cxxmph;
using namespace std;

int main(int argc, char** argv) {

  vector<int> flat;
  vector<vector<int>> v;
  v.push_back(vector<int>());
  for (int i = 0; i < 100; ++i) {
    v.push_back(vector<int>());
    for (int j = 0; j < 100; ++j) {
      v[i].push_back(j);
    }
  }
  flat.clear();
  for (int i = 0; i < v.size(); ++i) {
    for (int j = 0; j < v[i].size(); ++j) {
      flat.push_back(v[i][j]);
    }
  }
  auto begin = make_flatten_begin(&v);
  auto end = make_flatten_end(&v);
  int i = 0;
  for (auto it = begin; it != end; ++it, ++i) {
    if (i > flat.size()) { fprintf(stderr, "size error\n");  exit(-1); }
    if (flat[i] != *it) {
      fprintf(stderr, "expected: %d got %d\n", flat[i], *it);
      exit(-1);
    }
  }

  v[v.size()-1].clear();
  flat.clear();
  for (int i = 0; i < v.size(); ++i) {
    for (int j = 0; j < v[i].size(); ++j) {
      flat.push_back(v[i][j]);
    }
  }
  begin = make_flatten_begin(&v);
  end = make_flatten_end(&v);
  i = 0;
  for (auto it = begin; it != end; ++it, ++i) {
    if (i > flat.size()) { fprintf(stderr, "size error\n");  exit(-1); }
    if (flat[i] != *it) {
      fprintf(stderr, "expected: %d got %d\n", flat[i], *it);
      exit(-1);
    }
  }

  v[v.size()/2].clear();
  flat.clear();
  for (int i = 0; i < v.size(); ++i) {
    for (int j = 0; j < v[i].size(); ++j) {
      flat.push_back(v[i][j]);
    }
  }
  begin = make_flatten_begin(&v);
  end = make_flatten_end(&v);
  i = 0;
  for (auto it = begin; it != end; ++it, ++i) {
    if (i > flat.size()) { fprintf(stderr, "size error\n");  exit(-1); }
    if (flat[i] != *it) {
      fprintf(stderr, "expected: %d got %d\n", flat[i], *it);
      exit(-1);
    }
  }


  
  flatten_const_iterator<vector<vector<int>>> const_begin(begin);
  flatten_const_iterator<vector<vector<int>>> const_end(end);
  i = 0;
  for (auto it = const_begin; it != const_end; ++it, ++i) {
    if (flat[i] != *it) exit(-1);
  }
  auto it1 = make_flatten_begin(&v);
  auto it2 = make_flatten_begin(&v);
  if (it1 != it2) exit(-1);

  flatten_iterator<vector<vector<int>>> default_constructed;
  default_constructed = flatten_iterator<vector<vector<int>>>(&v, v.begin());

  vector<vector<pair<string, string>>> vs;
  vector<vector<pair<string, string>>>::iterator myot;
  vector<pair<string, string>>::iterator myit;
  auto fit = make_flatten(&vs, myot, myit);
}
