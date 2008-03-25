#ifndef __CMPH_TYPES_H__
#define __CMPH_TYPES_H__

typedef unsigned char cmph_uint8;
typedef unsigned short cmph_uint16;
typedef unsigned int cmph_uint32;
typedef float cmph_float32;

typedef enum { CMPH_HASH_JENKINS, CMPH_HASH_COUNT } CMPH_HASH;
extern const char *cmph_hash_names[];
typedef enum {	CMPH_BMZ, CMPH_BMZ8, CMPH_CHM, CMPH_BRZ, CMPH_FCH, 
				CMPH_BDZ, CMPH_BDZ_PH, CMPH_COUNT } CMPH_ALGO; /* included -- Fabiano */
extern const char *cmph_names[];

#endif
