#ifndef __CMPH_TYPES_H__
#define __CMPH_TYPES_H__

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint32;

typedef enum { HASH_JENKINS, HASH_DJB2, HASH_SDBM, HASH_FNV, HASH_GLIB, HASH_PJW, HASH_COUNT } CMPH_HASH;
extern const char *hash_names[];
typedef enum { MPH_CZECH, MPH_BMZ, MPH_COUNT } MPH_ALGO; /* included -- Fabiano */
extern const char *mph_names[];

#endif
