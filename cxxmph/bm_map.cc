#include <string>
#include <tr1/unordered_map>

#include "bm_common.h"
#include "mph_map.h"

using cxxmph::mph_map;
using std::string;
using std::tr1::unordered_map;

namespace cxxmph {

uint64_t myfind(const unordered_map<uint64_t, uint64_t, Murmur2>& mymap, const uint64_t& k) {
  return mymap.find(k)->second;
}
uint64_t myfind(const mph_map<uint64_t, uint64_t>& mymap, const uint64_t& k) {
  return mymap.index(k);
}

const StringPiece& myfind(const unordered_map<StringPiece, StringPiece, Murmur2StringPiece>& mymap, const StringPiece& k) {
  return mymap.find(k)->second;
}
StringPiece myfind(const mph_map<StringPiece, StringPiece>& mymap, const StringPiece& k) {
  auto it = mymap.find(k);
  return it->second;
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
  BM_SearchUrls(const std::string& urls_file, int nsearches) 
      : SearchUrlsBenchmark(urls_file, nsearches) { }
  virtual void Run() {
    for (auto it = random_.begin(); it != random_.end(); ++it) {
      auto idx = myfind(mymap_, *it);
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
  BM_SearchUint64() : SearchUint64Benchmark(1000*1000, 1000*1000) { }
  virtual bool SetUp() {
    if (!SearchUint64Benchmark::SetUp()) return false;
    for (int i = 0; i < values_.size(); ++i) {
      mymap_[values_[i]] = values_[i];
    }
    mymap_.rehash(mymap_.bucket_count());
    return true;
  }
  virtual void Run() {
    for (auto it = random_.begin(); it != random_.end(); ++it) {
      auto v = myfind(mymap_, *it);
    }
  }
  MapType mymap_;
};

}  // namespace cxxmph

using namespace cxxmph;

int main(int argc, char** argv) {
  /*
  Benchmark::Register(new BM_CreateUrls<mph_map<StringPiece, StringPiece>>("URLS100k"));
  Benchmark::Register(new BM_CreateUrls<unordered_map<StringPiece, StringPiece>>("URLS100k"));
  */
  Benchmark::Register(new BM_SearchUrls<mph_map<StringPiece, StringPiece>>("URLS100k", 1000* 1000*100));
  /*
  Benchmark::Register(new BM_SearchUrls<unordered_map<StringPiece, StringPiece, Murmur2StringPiece>>("URLS100k", 1000* 1000));
  Benchmark::Register(new BM_SearchUint64<unordered_map<uint64_t, uint64_t, Murmur2>>);
  Benchmark::Register(new BM_SearchUint64<mph_map<uint64_t, uint64_t>>);
  */
  Benchmark::RunAll();
}
