#include <fstream>
#include <iostream>
#include <set>

#include "bm_common.h"

using std::cerr;
using std::endl;
using std::set;
using std::string;
using std::vector;

namespace cxxmph {
  
bool UrlsBenchmark::SetUp() {
  vector<string> urls;
  std::ifstream f(urls_file_.c_str());
  if (!f.is_open()) {
    cerr << "Failed to open urls file " << urls_file_ << endl;
    return false;
  }
  string buffer;
  while(std::getline(f, buffer)) urls.push_back(buffer);
  set<string> unique(urls.begin(), urls.end());
  if (unique.size() != urls.size()) {
    cerr << "Input file has repeated keys." << endl;
    return false;
  }
  urls.swap(urls_);
  return true;
}

bool SearchUrlsBenchmark::SetUp() {
  if (!UrlsBenchmark::SetUp()) return false;
  random_.resize(nsearches_);
  for (int i = 0; i < nsearches_; ++i) {
    random_[i] = urls_[random() % urls_.size()];
  }
  return true;
}

bool Uint64Benchmark::SetUp() {
  set<uint64_t> unique;
  for (int i = 0; i < count_; ++i) {
    uint64_t v;
    do { v = random(); } while (unique.find(v) != unique.end());
    values_.push_back(v);
    unique.insert(v);
  }
  return true;
}

bool SearchUint64Benchmark::SetUp() {
  if (!Uint64Benchmark::SetUp()) return false;
  random_.resize(nsearches_);
  for (int i = 0; i < nsearches_; ++i) {
    random_.push_back(values_[random() % values_.size()]);
  }
  return true;
}

}  // namespace cxxmph 
