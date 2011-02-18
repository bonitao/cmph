#include <set>
#include <vector>

#include "benchmark.h"
#include "mphtable.h"

using std::set;
using std::vector;

namespace cxxmph {
class BM_NumbersCreate : public Benchmark {
 public:
  BM_NumbersCreate(int iters = 1) : Benchmark(iters) {
    set<int> unique;
    while (unique.size() < 1000 * 1000) {
      int v = random();
      if (unique.find(v) == unique.end()) {
        unique.insert(v);
        random_unique_.push_back(v);
      }
    }
  }
 protected:
  virtual void Run(int iters) {
    SimpleMPHTable<int> table;
    table.Reset(random_unique_.begin(), random_unique_.end());
  }
  std::vector<int> random_unique_;
};

class BM_NumbersFind : public BM_NumbersCreate {
 public:
  BM_NumbersFind(int iters) : BM_NumbersCreate(iters) { table_.Reset(random_unique_.begin(), random_unique_.end()); } 
  virtual void Run(int iters) {
    for (int i = 0; i < iters * 100; ++i) {
      int pos = random() % random_unique_.size();;
      int h = table_.index(pos);
    }
  }
 private:
   SimpleMPHTable<int> table_;
};

}  // namespace cxxmph

using namespace cxxmph;

int main(int argc, char** argv) {
  Benchmark::Register(new BM_NumbersCreate());
  Benchmark::Register(new BM_NumbersFind(1000 * 1000));
  Benchmark::RunAll();
}
