#include <set>
#include <string>
#include <tr1/unordered_map>

#include "bm_common.h"
#include "stringpiece.h"
#include "mph_index.h"

using namespace cxxmph;

using std::string;
using std::tr1::unordered_map;

class BM_MPHIndexCreate : public UrlsBenchmark {
 public:
  BM_MPHIndexCreate(const std::string& urls_file)
      : UrlsBenchmark(urls_file) { }
 protected:
  virtual void Run() {
    SimpleMPHIndex<StringPiece> index;
    index.Reset(urls_.begin(), urls_.end());
  }
};

class BM_STLIndexCreate : public UrlsBenchmark {
 public:
  BM_STLIndexCreate(const std::string& urls_file)
      : UrlsBenchmark(urls_file) { }
 protected:
  virtual void Run() {
    unordered_map<StringPiece, uint32_t> index;
    int idx = 0;
    for (auto it = urls_.begin(); it != urls_.end(); ++it) {
      index.insert(make_pair(*it, idx++));
    }
  }
};
   
class BM_MPHIndexSearch : public SearchUrlsBenchmark {
 public:
  BM_MPHIndexSearch(const std::string& urls_file, int nsearches)
      : SearchUrlsBenchmark(urls_file, nsearches) { }
  virtual void Run() {
    for (auto it = random_.begin(); it != random_.end(); ++it) {
      auto idx = index_.index(*it);
      // Collision check to be fair with STL
      if (strcmp(urls_[idx].c_str(), it->data()) != 0) idx = -1;
    }
  }
 protected:
  virtual bool SetUp () {
   if (!SearchUrlsBenchmark::SetUp()) return false;
   index_.Reset(urls_.begin(), urls_.end());
   return true;
  }
  SimpleMPHIndex<StringPiece> index_;
};

class BM_STLIndexSearch : public SearchUrlsBenchmark {
 public:
  BM_STLIndexSearch(const std::string& urls_file, int nsearches)
      : SearchUrlsBenchmark(urls_file, nsearches) { }
  virtual void Run() {
    for (auto it = random_.begin(); it != random_.end(); ++it) {
      auto idx = index_.find(*it);
    }
  }
 protected:
  virtual bool SetUp () {
   if (!SearchUrlsBenchmark::SetUp()) return false;
   unordered_map<StringPiece, uint32_t> index;
   int idx = 0;
   for (auto it = urls_.begin(); it != urls_.end(); ++it) {
     index.insert(make_pair(*it, idx++));
   }
   index.swap(index_);
   return true;
  }
  std::tr1::unordered_map<StringPiece, uint32_t> index_;
};

int main(int argc, char** argv) {
  Benchmark::Register(new BM_MPHIndexCreate("URLS100k"));
  Benchmark::Register(new BM_STLIndexCreate("URLS100k"));
  Benchmark::Register(new BM_MPHIndexSearch("URLS100k", 100*1000*1000));
  Benchmark::Register(new BM_STLIndexSearch("URLS100k", 100*1000*1000));
  Benchmark::RunAll();
  return 0;
}
