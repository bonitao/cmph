#ifndef _CMPH_BITBOOL_H__
#define _CMPH_BITBOOL_H__
#include "cmph_types.h" 
extern const cmph_uint8 bitmask[];
#define GETBIT(array, i) (array[(i) / 8] & bitmask[(i) % 8])
#define SETBIT(array, i) (array[(i) / 8] |= bitmask[(i) % 8])
#define UNSETBIT(array, i) (array[(i) / 8] &= (~(bitmask[(i) % 8])))

#endif
