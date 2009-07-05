#ifndef __CMPH_CDB_H__
#define __CMPH_CDB_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct __cdb_t cdb;
typedef struct __cdb_make_t cdb_make;

int cdb_init(cdb *cdbp, int fd);
void cdb_free(cdb *cdbp);
int cdb_read(const cdb *cdbp, void *buf, cmph_uint32 len, cmph_uint32 pos);
int cdb_find(const cdb *cdbp, const void *key, cmph_uint32 keylen);
int cdb_read(const cdb *cdbp, void *buf, cmph_uint32 len, cmph_uint32 pos);

int cdb_make_start(cdb_make *cdbmp, int fd);
int cdb_make_add(cdb_make *cdbmp, const void *key, cmph_uint32 keylen, const void *val, cmph_uint32 vallen);
int cdb_make_finish(cdb_make *cdbmp);


#ifdef __cplusplus
}
#endif

#endif
