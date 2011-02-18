#ifndef __CXXMPH_BENCHMARK_H__
#define __CXXMPH_BENCHMARK_H__

#include <string>
#include <typeinfo>

namespace cxxmph {

class Benchmark {
 public:
  Benchmark(int iters = 1) : iters_(iters) { }
  virtual void Run(int iters) = 0;
  virtual ~Benchmark() { }
  const std::string& name() { return name_; }
  void set_name(const std::string& name) { name_ = name; }

  static void Register(Benchmark* bm);
  static void RunAll();

 protected:
  int iters() { return iters_; }

 private:
  int iters_;
  std::string name_;
  void MeasureRun();
};

}  // namespace cxxmph

#endif
