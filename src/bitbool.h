#ifndef _CMPH_BITBOOL_H__
#define _CMPH_BITBOOL_H__
#include "cmph_types.h" 
extern const cmph_uint8 bitmask[];

#define GETBIT(array, i) ((array[i >> 3] & bitmask[i & 0x00000007]) >> (i & 0x00000007))
#define SETBIT(array, i) (array[i >> 3] |= bitmask[i & 0x00000007])
#define UNSETBIT(array, i) (array[i >> 3] &= (~(bitmask[i & 0x00000007])))

//#define GETBIT(array, i) (array[(i) / 8] & bitmask[(i) % 8])
//#define SETBIT(array, i) (array[(i) / 8] |= bitmask[(i) % 8])
//#define UNSETBIT(array, i) (array[(i) / 8] &= (~(bitmask[(i) % 8])))

extern const cmph_uint8 valuemask[];
#define SETVALUE(array, i, v) (array[i >> 2] &= ((v << ((i & 0x00000003) << 1)) | valuemask[i & 0x00000003]))
#define GETVALUE(array, i) ((array[i >> 2] >> ((i & 0x00000003) << 1)) & 0x00000003)

#endif
