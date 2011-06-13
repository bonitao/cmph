#include <set>
#include <string>
#include <tr1/unordered_set>

#include "bm_common.h"
#include "StringPiece.h"
#include "mph_index.h"

using namespace cxxmph;

using std::string;
using std::tr1::unordered_set;

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
    unordered_set<StringPiece> index;
    index.insert(urls_.begin(), urls_.end());
  }
};
   
class BM_MPHIndexSearch : public SearchUrlsBenchmark {
 public:
  BM_MPHIndexSearch(const std::string& urls_file, int nsearches)
      : SearchUrlsBenchmark(urls_file, nsearches) { }
  virtual void Run() {
    while (true) {
    for (auto it = random_.begin(); it != random_.end(); ++it) {
      index_.index(*it);
    }
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
      index_.find(*it); // - index_.begin();
    }
  }
 protected:
  virtual bool SetUp () {
   if (!SearchUrlsBenchmark::SetUp()) return false;
   std::tr1::unordered_set<StringPiece>(urls_.begin(), urls_.end()).swap(index_);
   return true;
  }
  std::tr1::unordered_set<StringPiece> index_;
};

int main(int argc, char** argv) {
  Benchmark::Register(new BM_MPHIndexCreate("URLS100k"));
  Benchmark::Register(new BM_STLIndexCreate("URLS100k"));
  Benchmark::Register(new BM_MPHIndexSearch("URLS100k", 1000*1000));
  Benchmark::Register(new BM_STLIndexSearch("URLS100k", 1000*1000));
  Benchmark::RunAll();
  return 0;
}
