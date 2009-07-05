#ifndef __CMPH_CDB_H__
#define __CMPH_CDB_H__

#ifdef __cplusplus
extern "C"
{
#endif

typedef struct
{
	int fd;
	void *data;
} cdb;

int cdb_init(struct cdb *cdbp, int fd);
void cdb_free(struct cdb *cdbp);
int cdb_read(const struct cdb *cdbp, void *buf, cmph_uint32 len, cmph_uint32 pos);
int cdb_find(const struct cdb *cdbp, const void *key, cmph_uint32 keylen);
int cdb_read(const struct cdb *cdbp, void *buf, cmph_uint32 len, cmph_uint32 pos);

typedef struct
{
	int fd;
	void *data;
} cdb_make;

int cdb_make_start(struct cdb_make *cdbmp, int fd);
int cdb_make_add(struct cdb_make *cdbmp, const void *key, cmph_uint32 keylen, const void *val, cmph_uint32 vallen);
int cdb_make_exists(struct cdb_make *cdbmp, const void *key, cmph_uint32 klen);
int cdb_make_find(struct cdb_make *cdbmp, const void *key, cmph_uint32 klen, enum cdb_put_mode mode);
int cdb_make_put(struct cdb_make *cdbmp, const void *key, cmph_uint32 klen, const void *val, cmph_uint32 vlen, enum cdb_put_mode mode);
int cdb_make_finish(struct cdb_make *cdbmp);


#ifdef __cplusplus
}
#endif

#endif
