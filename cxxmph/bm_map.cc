#include <string>
#include <tr1/unordered_map>

#include "bm_common.h"
#include "mph_map.h"

using cxxmph::mph_map;
using std::string;
using std::unordered_map;

namespace cxxmph {

template<class MapType, class T>
const T* myfind(const MapType& mymap, const T& k) {
  auto it = mymap.find(k);
  auto end = mymap.end();
  if (it == end) return NULL;
  return &it->second;
}

template <class MapType>
class BM_CreateUrls : public UrlsBenchmark {
 public:
  BM_CreateUrls(const string& urls_file) : UrlsBenchmark(urls_file) { }
  virtual void Run() {
    MapType mymap;
    for (auto it = urls_.begin(); it != urls_.end(); ++it) {
      mymap[*it] = *it;
    }
  }
};

template <class MapType>
class BM_SearchUrls : public SearchUrlsBenchmark {
 public:
  BM_SearchUrls(const std::string& urls_file, int nsearches, float miss_ratio) 
      : SearchUrlsBenchmark(urls_file, nsearches, miss_ratio) { }
  virtual void Run() {
    for (auto it = random_.begin(); it != random_.end(); ++it) {
      auto v = myfind(mymap_, *it);
      assert(it->ends_with(".force_miss") ^ v != NULL);
      assert(!v || *v == *it);
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

template <class MapType>
class BM_SearchUint64 : public SearchUint64Benchmark {
 public:
  BM_SearchUint64() : SearchUint64Benchmark(10000, 10*1000*1000) { }
  virtual bool SetUp() {
    if (!SearchUint64Benchmark::SetUp()) return false;
    for (int i = 0; i < values_.size(); ++i) {
      mymap_[values_[i]] = values_[i];
    }
    mymap_.rehash(mymap_.bucket_count());
    // Double check if everything is all right
    for (int i = 0; i < values_.size(); ++i) {
      if (mymap_[values_[i]] != values_[i]) return false;
    }
    return true;
  }
  virtual void Run() {
    for (auto it = random_.begin(); it != random_.end(); ++it) {
      auto v = myfind(mymap_, *it);
      if (*v != *it) {
        fprintf(stderr, "Looked for %lu got %lu\n", *it, *v);
	exit(-1);
      }
    }
  }
  MapType mymap_;
};

}  // namespace cxxmph

using namespace cxxmph;

int main(int argc, char** argv) {
  srandom(4);
  Benchmark::Register(new BM_CreateUrls<mph_map<StringPiece, StringPiece>>("URLS100k"));
  Benchmark::Register(new BM_CreateUrls<unordered_map<StringPiece, StringPiece>>("URLS100k"));
  Benchmark::Register(new BM_SearchUrls<mph_map<StringPiece, StringPiece>>("URLS100k", 10*1000 * 1000, 0));
  Benchmark::Register(new BM_SearchUrls<unordered_map<StringPiece, StringPiece, Murmur3StringPiece>>("URLS100k", 10*1000 * 1000, 0));
  Benchmark::Register(new BM_SearchUrls<mph_map<StringPiece, StringPiece>>("URLS100k", 10*1000 * 1000, 0.9));
  Benchmark::Register(new BM_SearchUrls<unordered_map<StringPiece, StringPiece, Murmur3StringPiece>>("URLS100k", 10*1000 * 1000, 0.9));
  Benchmark::Register(new BM_SearchUint64<unordered_map<uint64_t, uint64_t>>);
  Benchmark::Register(new BM_SearchUint64<mph_map<uint64_t, uint64_t>>);
  Benchmark::RunAll();
}
