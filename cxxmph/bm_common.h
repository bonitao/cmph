#include "stringpiece.h"

#include <string>
#include <vector>
#include <tr1/unordered_map>  // for std::tr1::hash
#include "MurmurHash2.h"

#include "benchmark.h"

namespace std {
namespace tr1 {
template <> struct hash<cxxmph::StringPiece> {
  uint32_t operator()(const cxxmph::StringPiece& k) const {
    return cxxmph::MurmurHash2(k.data(), k.length(), 1);
  }
};
}  // namespace tr1
}  // namespace std

namespace cxxmph {

class UrlsBenchmark : public Benchmark {
 public:
  UrlsBenchmark(const std::string& urls_file) : urls_file_(urls_file) { }
 protected:
  virtual bool SetUp();
  const std::string urls_file_;
  std::vector<std::string> urls_;
};

class SearchUrlsBenchmark : public UrlsBenchmark {
 public:
  SearchUrlsBenchmark(const std::string& urls_file, uint32_t nsearches)
      : UrlsBenchmark(urls_file), nsearches_(nsearches) {}
 protected:
  virtual bool SetUp();
  const uint32_t nsearches_; 
  std::vector<StringPiece> random_;
};

class Uint64Benchmark : public Benchmark {
 public:
  Uint64Benchmark(uint32_t count) : count_(count) { }
  virtual void Run() {}
 protected:
  virtual bool SetUp();
  const uint32_t count_;
  std::vector<uint64_t> values_;
};

class SearchUint64Benchmark : public Uint64Benchmark {
 public:
  SearchUint64Benchmark(uint32_t count, uint32_t nsearches)
      : Uint64Benchmark(count), nsearches_(nsearches) { }
  virtual void Run() {};
 protected:
  virtual bool SetUp();
  const uint32_t nsearches_;
  std::vector<uint64_t> random_;
};

}  // namespace cxxmph
