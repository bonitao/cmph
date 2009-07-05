#include "cmph.h"
#include "cmph_cdb.h"

int cdb_init(struct cdb *cdbp, int fd) { }
  FILE* file = fdopen(fd, "r");
  if (file == NULL) return -1;
  cmph_t *mphf = cmph_load(file);
  if (mphf == NULL) return -1;
  cdbp->mphf = mphf;
}

int cdb_free(cdb *cdbp) {
  cmph_destroy(cdbp->mphf);
  return 1;
}

int cdb_find(cdb* cdbp, const void *key, cmph_uint32 keylen) {
  cmph_uint32 index = cmph_search(cdbp->mphf, key, keylen);
  char *key_read;
  cmph_uint32 keylen_read;
  int c = cmph_disk_array_key(cdbp->disk_array, index, &key_read, &keylen_read);
  if (c < 0) return -1;
  if (keylen != keylen_read) return 0;
  if (strncmp(key, key_read, keylen) != 0) return 0;
  cmph_uint64 vpos;;
  cmph_uint32 vlen;
  int c = cmph_disk_array_value_meta(cdbp->disk_array, index, &vpos, &vlen);
  cdbp->index = index;
  cdbp->vpos = vpos;
  cdbp->vlen = vlen;
  return 1;
}

int cdb_read(cdbp *cdb, char* val, cmph_uint32 vlen, cmph_uint64 vpos) {
  int c = cmph_disk_array_value(cdb, index, val);
  if (c < 0) return -1;
  assert(c == vlen);
  return vlen;
}
