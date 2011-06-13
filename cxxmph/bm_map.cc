#include <string>
#include <tr1/unordered_map>

#include "bm_common.h"
#include "mph_map.h"

using cxxmph::mph_map;
using std::string;
using std::tr1::unordered_map;

namespace cxxmph {

template <class MapType>
class BM_MapCreate : public UrlsBenchmark {
 public:
  BM_MapCreate(const string& urls_file) : UrlsBenchmark(urls_file) { }
  virtual void Run() {
    MapType mymap;
    for (auto it = urls_.begin(); it != urls_.end(); ++it) {
      mymap[*it] = *it;
    }
  }
};

template <class MapType>
class BM_MapSearch : public SearchUrlsBenchmark {
 public:
  BM_MapSearch(const std::string& urls_file, int nsearches) 
      : SearchUrlsBenchmark(urls_file, nsearches) { }
  virtual void Run() {
    for (auto it = random_.begin(); it != random_.end(); ++it) {
      mymap_.find(*it);
    }
  }
 protected:
  virtual bool SetUp() {
    if (!SearchUrlsBenchmark::SetUp()) return false;
    for (auto it = urls_.begin(); it != urls_.end(); ++it) {
      mymap_[*it] = *it;
    }
    mymap_.rehash(mymap_.bucket_count());
    return true;
  }
  MapType mymap_;
};

}  // namespace cxxmph

using namespace cxxmph;

int main(int argc, char** argv) {
  Benchmark::Register(new BM_MapCreate<mph_map<StringPiece, StringPiece>>("URLS100k"));
  Benchmark::Register(new BM_MapCreate<unordered_map<StringPiece, StringPiece>>("URLS100k"));
  Benchmark::Register(new BM_MapSearch<mph_map<StringPiece, StringPiece>>("URLS100k", 1000* 1000));
  Benchmark::Register(new BM_MapSearch<unordered_map<StringPiece, StringPiece>>("URLS100k", 1000* 1000));
  Benchmark::RunAll();
}
