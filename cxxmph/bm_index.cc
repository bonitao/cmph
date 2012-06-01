#include <cmph.h>

#include <cstdio>
#include <set>
#include <string>
#include <unordered_map>

#include "bm_common.h"
#include "stringpiece.h"
#include "mph_index.h"

using namespace cxxmph;

using std::string;
using std::unordered_map;

class BM_MPHIndexCreate : public UrlsBenchmark {
 public:
  BM_MPHIndexCreate(const std::string& urls_file)
      : UrlsBenchmark(urls_file) { }
 protected:
  virtual void Run() {
    SimpleMPHIndex<StringPiece> index;
    index.Reset(urls_.begin(), urls_.end(), urls_.size());
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
      : SearchUrlsBenchmark(urls_file, nsearches, 0) { }
  virtual void Run() {
    uint64_t sum = 0;
    for (auto it = random_.begin(); it != random_.end(); ++it) {
      auto idx = index_.index(*it);
      // Collision check to be fair with STL
      if (strcmp(urls_[idx].c_str(), it->data()) != 0) idx = -1;
      sum += idx;
    }
  }
 protected:
  virtual bool SetUp () {
   if (!SearchUrlsBenchmark::SetUp()) return false;
   index_.Reset(urls_.begin(), urls_.end(), urls_.size());
   return true;
  }
  SimpleMPHIndex<StringPiece> index_;
};

class BM_CmphIndexSearch : public SearchUrlsBenchmark {
 public:
  BM_CmphIndexSearch(const std::string& urls_file, int nsearches)
      : SearchUrlsBenchmark(urls_file, nsearches, 0) { }
  ~BM_CmphIndexSearch() { if (index_) cmph_destroy(index_); }
  virtual void Run() {
    uint64_t sum = 0;
    for (auto it = random_.begin(); it != random_.end(); ++it) {
      auto idx = cmph_search(index_, it->data(), it->length());
      // Collision check to be fair with STL
      if (strcmp(urls_[idx].c_str(), it->data()) != 0) idx = -1;
      sum += idx;
    }
  }
 protected:
  virtual bool SetUp() {
   if (!SearchUrlsBenchmark::SetUp()) {
      cerr << "Parent class setup failed." << endl;
      return false;
    }
    FILE* f = fopen(urls_file_.c_str(), "r");
    if (!f) {
      cerr << "Faied to open " << urls_file_ << endl; 
      return false;
    }
    cmph_io_adapter_t* source = cmph_io_nlfile_adapter(f);
    if (!source) {
      cerr << "Faied to create io adapter for " << urls_file_ << endl; 
      return false;
    }
    cmph_config_t* config = cmph_config_new(source);
    if (!config) {
      cerr << "Failed to create config" << endl;
      return false;
    }
    cmph_config_set_algo(config, CMPH_BDZ);
    cmph_t* mphf = cmph_new(config);
    if (!mphf) {
      cerr << "Failed to create mphf." << endl;
      return false;
    }

    cmph_config_destroy(config);
    cmph_io_nlfile_adapter_destroy(source);
    fclose(f);
    index_ = mphf;
    return true;
  }
  cmph_t* index_;
};
    

class BM_STLIndexSearch : public SearchUrlsBenchmark {
 public:
  BM_STLIndexSearch(const std::string& urls_file, int nsearches)
      : SearchUrlsBenchmark(urls_file, nsearches, 0) { }
  virtual void Run() {
    uint64_t sum = 0;
    for (auto it = random_.begin(); it != random_.end(); ++it) {
      auto idx = index_.find(*it);
      sum += idx->second;
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
  unordered_map<StringPiece, uint32_t> index_;
};

int main(int argc, char** argv) {
  Benchmark::Register(new BM_MPHIndexCreate("URLS100k"));
  Benchmark::Register(new BM_STLIndexCreate("URLS100k"));
  Benchmark::Register(new BM_MPHIndexSearch("URLS100k", 10*1000*1000));
  Benchmark::Register(new BM_STLIndexSearch("URLS100k", 10*1000*1000));
  Benchmark::Register(new BM_CmphIndexSearch("URLS100k", 10*1000*1000));
  Benchmark::RunAll();
  return 0;
}
