#include "benchmark.h"

#include <cstring>
#include <cstdio>
#include <sys/resource.h>

#include <iostream>
#include <vector>

using std::cerr;
using std::endl;
using std::string;
using std::vector;

namespace {

/* Subtract the `struct timeval' values X and Y,
   storing the result in RESULT.
   Return 1 if the difference is negative, otherwise 0.  */
int timeval_subtract ( 
    struct timeval *result, struct timeval *x, struct timeval* y) {
  /* Perform the carry for the later subtraction by updating y. */
  if (x->tv_usec < y->tv_usec) {
    int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
    y->tv_usec -= 1000000 * nsec;
    y->tv_sec += nsec;
  }
  if (x->tv_usec - y->tv_usec > 1000000) {
    int nsec = (x->tv_usec - y->tv_usec) / 1000000;
    y->tv_usec += 1000000 * nsec;
    y->tv_sec -= nsec;
  }

  /* Compute the time remaining to wait.
     tv_usec is certainly positive. */
  result->tv_sec = x->tv_sec - y->tv_sec;
  result->tv_usec = x->tv_usec - y->tv_usec;

  /* Return 1 if result is negative. */
  return x->tv_sec < y->tv_sec;
}

struct rusage getrusage_or_die() {
  struct rusage rs;
  int ret = getrusage(RUSAGE_SELF, &rs);
  if (ret != 0) {
    cerr << "rusage failed: " << strerror(errno) << endl;
    exit(-1);
  }
  return rs;
}

#ifdef HAVE_CXA_DEMANGLE
string demangle(const string& name) {
  char buf[1024];
  unsigned int size = 1024;
  int status;
  char* res = abi::__cxa_demangle(
     name.c_str(), buf, &size, &status);
  return res;
}
#else
string demangle(const string& name) { return name; }
#endif
 

static vector<cxxmph::Benchmark*> g_benchmarks;

}  // anonymous namespace

namespace cxxmph {

/* static */ void Benchmark::Register(Benchmark* bm) {
  if (bm->name().empty()) {
    string name = demangle(typeid(*bm).name());
     bm->set_name(name);
  }
  g_benchmarks.push_back(bm);
}

/* static */ void Benchmark::RunAll() {
  for (auto it = g_benchmarks.begin(); it != g_benchmarks.end(); ++it) {
    (*it)->MeasureRun();
    delete *it;
  }
}

void Benchmark::MeasureRun() {
  struct rusage begin = getrusage_or_die();
  Run(iters_);
  struct rusage end = getrusage_or_die();

  struct timeval utime;
  timeval_subtract(&utime, &end.ru_utime, &begin.ru_utime);
  struct timeval stime;
  timeval_subtract(&stime, &end.ru_stime, &begin.ru_stime);

  printf("Benchmark: %s\n", name().c_str());
  printf("User time used  : %ld.%06ld\n", utime.tv_sec, utime.tv_usec);
  printf("System time used: %ld.%06ld\n", stime.tv_sec, stime.tv_usec);
  printf("\n");
}

}  // namespace cxxmph
