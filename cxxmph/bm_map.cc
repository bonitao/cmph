#include <string>
#include <unordered_map>

#include "bm_common.h"
#include "mph_map.h"

using cxxmph::mph_map;
using std::string;
using std::unordered_map;

namespace cxxmph {

template <class MapType>
class BM_MapCreate : public UrlsBenchmark {
 public:
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
  virtual void Run() {
    for (auto it = random_.begin(); it != random_.end(); ++it) {
      auto value = mymap[*it];
    }
  }
 protected:
  virtual void SetUp() {
    for (auto it = urls_.begin(); it != urls_.end(); ++it) {
      mymap_[*it] = *it;
    }
    mymap_.resize(mymap.size());
  }
  MapType mymap_;
};

}  // namespace cxxmph

using namespace cxxmph;

int main(int argc, char** argv) {
  Benchmark::Register(new BM_MapCreate<mph_map<string, string>>("URLS100k"));
  Benchmark::Register(new BM_MapCreate<unordered_map<string, string>>("URLS100k"));
  Benchmark::Register(new BM_MapSearch<mph_map<string, string>>("URLS100k", 1000 * 1000));
  Benchmark::Register(new BM_MapSearch<unordered_map<string, string>>("URLS100k", 1000 * 1000));
  Benchmark::RunAll();
}
