#include <cstdlib>
#include <cstdio>
#include <vector>
#include <iostream>
#include <utility>


using std::cerr;
using std::endl;
using std::vector;
using std::pair;
using std::make_pair;

#include "hollow_iterator.h"

int main(int, char**) {
  vector<int> v;
  vector<bool> p;
  for (int i = 0; i < 100; ++i) {
    v.push_back(i);
    p.push_back(i % 2 == 0);
  }
  auto begin = make_hollow(&v, &p, v.begin());
  auto end = make_hollow(&v, &p, v.end());
  for (auto it = begin; it != end; ++it) {
    if (((*it) % 2) != 0) exit(-1);
  }
  const vector<int>* cv(&v);
  auto cbegin(make_hollow(cv, &p, cv->begin()));
  auto cend(make_hollow(cv, &p, cv->begin()));
  for (auto it = cbegin; it != cend; ++it) {
    if (((*it) % 2) != 0) exit(-1);
  }
  const vector<bool>* cp(&p);
  cbegin = make_hollow(cv, cp, v.begin());
  cend = make_hollow(cv, cp, cv->end());

  vector<int>::iterator vit1 = v.begin();
  vector<int>::const_iterator vit2 = v.begin();
  if (vit1 != vit2) exit(-1);
  auto it1 = make_hollow(&v, &p, vit1);
  auto it2 = make_hollow(&v, &p, vit2);
  if (it1 != it2) exit(-1);

  typedef is_empty<const vector<int>> iev;
  hollow_iterator_base<vector<int>::iterator, iev> default_constructed;
  default_constructed = make_hollow(&v, &p, v.begin());
  return 0;
  
  vector<pair<int, pair<int, int>>> vp;
  vp.push_back(make_pair(5,make_pair(0, 1)));
  auto pbegin = make_iterator_second(make_hollow(&vp, &p, vp.begin()));
  auto pend = make_iterator_second(make_hollow(&vp, &p, vp.end()));
  if (pbegin->second != pbegin->first + 1) exit(-1);
  while (pbegin != pend) ++pbegin;
  

}

