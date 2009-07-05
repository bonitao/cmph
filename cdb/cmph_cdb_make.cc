#include "cmph_cdb.h"

int cdb_make_start(cdb_make* cdbmp, int fd) {
  cdbmp->config = cmph_config_new(key_source);
  cdbmp->tmpfd = fd;
}

int cdb_make_add(cdb_make* cdbmp, const void* key, cmph_uint32 klen, const void *val, cmph_uint32 vlen) {
  /* Uses cdb format as documented at http://cr.yp.to/cdb/cdbmake.html. A 32 bits integer takes at most 11 bytes when represented as a string. Adding the plus sign, the comma, the colon and the zero terminator, a total of 25 bytes is required. We use 50. */
  char preamble[50];
  cmph_uint64 c = snprintf(preamble, 25, "+%u,%u:", klen, vlen);
  c += write(cdbmp->fd, preamble);
  c += write(cdbmp->fd, key);
  c += write(cdbmp->fd, ",");
  c += write(cdbmp->fd, value);
  c += write(cdbmp->fd, "\n");
  if (c != strlen(preamble) + strlen(key) + strlen(value) + 2) return -1;
}

int cdb_make_finish(cdb_make *cdbmp) {
  cmph_io_adapter_t *key_source = cmph_io_cdb_adapter(cdbmp->fd, cdbmp->nkeys);
  if (!key_source) return -1;
  cmph_config_t *config = cmph_config_new(key_source);
  if (!config) return -1;
  cmph_t* mphf = cmph_new(config);
  if (!mphf) return -1;
  FILE *f = fdopen(cdbmp->fd);
  if (!f) return -1;
  int c = cmph_dump(mphf, f);
  if (c < 0) return -1;
}
