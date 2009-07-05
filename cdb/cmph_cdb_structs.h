#ifndef __CMPH_CDB_STRUCTS_H__
#define __CMPH_CDB_STRUCTS_H__

struct __cmph_cdb_t
{
  cmph_t *mph;
  cmph_uint32 *idx;
  FILE* data;
  cmph_uint64 mmap_data_len;
  void* mmap_data;
};

struct __cmph_cdb_make_t {
  cmph_config_t* config;


#endif
