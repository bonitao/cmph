#include "cmph.h"
#include "cmph_benchmark.h"

void bm_bdz_numbers(int iters) {
  cmph_config_t config;
  config.algo = CMPH_BMZ;
  

int main(int argc, char** argv) {
  run_benchmarks(argc, argv);
}

