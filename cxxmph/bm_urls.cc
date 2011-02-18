#include <fstream>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include <unordered_map>

#include "benchmark.h"
#include "cmph_hash_map.h"

using std::ifstream;
using std::set;
using std::string;
using std::vector;

namespace cxxmph {

class BM_UrlsCreate : public Benchmark {
 public:
  BM_UrlsCreate(int iters = 1) : Benchmark(iters) {
    ReadUrls();
  }
 protected:
  virtual void Run(int iters) {
    BuildTable();
  }
  void BuildTable() {
    for (auto it = urls_.begin(); it != urls_.end(); ++it) {
      table_[*it] = it - urls_.begin();
    }
    table_.pack();
  }
  void ReadUrls() {
    vector<string> urls;
    std::ifstream f("URLS100k");
    string buffer;
    while(std::getline(f, buffer)) urls.push_back(buffer);
    set<string> unique(urls.begin(), urls.end());
    if (unique.size() != urls.size()) {
      cerr << "Input file has repeated keys." << endl;
      exit(-1);
    }
    urls_.swap(urls);
  }
  vector<string> urls_;
  cxxmph::cmph_hash_map<string, int> table_;
};
   
class BM_UrlsFind : public BM_UrlsCreate {
 public:
  BM_UrlsFind(int iters = 1) : BM_UrlsCreate(iters) { ReadUrls(); BuildTable(); }
 protected:
  virtual void Run(int iters) {
    for (int i = 0; i < iters * 100; ++i) {
      int pos = random() % urls_.size();;
      int h = table_[urls_[pos]];
      assert(h == pos);
    }
  }
};

}  // namespace cxxmph

using namespace cxxmph;

int main(int argc, char** argv) {
  Benchmark::Register(new BM_UrlsCreate());
  Benchmark::Register(new BM_UrlsFind(1000 * 1000));
  Benchmark::RunAll();
}
