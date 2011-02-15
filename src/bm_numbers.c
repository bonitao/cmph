#include <stdlib.h>
#include <string.h>

#include "bitbool.h"
#include "cmph.h"
#include "cmph_benchmark.h"
#include "linear_string_map.h"

// Generates a vector with random unique 32 bits integers
cmph_uint32* random_numbers_vector_new(cmph_uint32 size) {
  cmph_uint32 i = 0;
  cmph_uint32 dup_bits = sizeof(cmph_uint32)*size*8;
  char* dup = (char*)malloc(dup_bits/8);
  cmph_uint32* vec = (cmph_uint32 *)malloc(sizeof(cmph_uint32)*size);
  memset(dup, 0, dup_bits/8);
  for (i = 0; i < size; ++i) {
    cmph_uint32 v = random();
    while (GETBIT(dup, v % dup_bits)) { v = random(); }
    SETBIT(dup, v % dup_bits);
    vec[i] = v;
    fprintf(stderr, "v[%u] = %u\n", i, vec[i]);
  }
  free(dup);
  return vec;
}
static cmph_uint32 g_numbers_len = 0;
static cmph_uint32 *g_numbers = NULL;
static lsmap_t *g_created_mphf = NULL;

void bm_create(CMPH_ALGO algo, int iters) {
  cmph_uint32 i = 0;
  cmph_io_adapter_t* source = NULL;
  cmph_config_t* config = NULL;
  cmph_t* mphf = NULL;

  if (iters > g_numbers_len) {
    fprintf(stderr, "No input with proper size.");
    exit(-1);
  }

  source = cmph_io_struct_vector_adapter(
      (void*)g_numbers, sizeof(cmph_uint32),
      0, sizeof(cmph_uint32), iters);
  config = cmph_config_new(source);
  cmph_config_set_algo(config, algo);
  mphf = cmph_new(config);
  if (!mphf) {
    fprintf(stderr, "Failed to create mphf for algorithm %s with %u keys",
            cmph_names[algo], iters);
    exit(-1);
  }
  cmph_config_destroy(config);
  cmph_io_struct_vector_adapter_destroy(source);
  

  char mphf_name[128];
  snprintf(mphf_name, 128, "%s:%u", cmph_names[algo], iters);
  lsmap_append(g_created_mphf, strdup(mphf_name), mphf);
}

void bm_search(CMPH_ALGO algo, int iters) {
  int i = 0;
  char mphf_name[128];
  cmph_t* mphf = NULL; 

  snprintf(mphf_name, 128, "%s:%u", cmph_names[algo], iters);
  mphf = lsmap_search(g_created_mphf, mphf_name);
  for (i = 0; i < iters * 100; ++i) {
    cmph_uint32 pos = random() % iters;
    fprintf(stderr, "Looking for key %u at pos %u\n", g_numbers[pos], pos);
    const char* buf = (const char*)(g_numbers + pos);
    cmph_uint32 h = cmph_search(mphf, buf, sizeof(cmph_uint32));
    fprintf(stderr, "Found h %u value %u\n", h, g_numbers[h]);
    if (h != pos) {
      fprintf(stderr, "Buggy mphf\n");
    }
  }
}

#define DECLARE_ALGO(algo) \
  void bm_create_ ## algo(int iters) { bm_create(algo, iters); } \
  void bm_search_ ## algo(int iters) { bm_search(algo, iters); }

DECLARE_ALGO(CMPH_BDZ);

int main(int argc, char** argv) {
  g_numbers_len = 20;
  g_numbers = random_numbers_vector_new(g_numbers_len);
  g_created_mphf = lsmap_new();

  BM_REGISTER(bm_create_CMPH_BDZ, 20);
  BM_REGISTER(bm_search_CMPH_BDZ, 20);
  run_benchmarks(argc, argv);

  free(g_numbers);
  lsmap_foreach_key(g_created_mphf, free);
  lsmap_foreach_value(g_created_mphf, cmph_destroy);
  lsmap_destroy(g_created_mphf);
}

