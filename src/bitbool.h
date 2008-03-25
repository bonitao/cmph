#ifndef _CMPH_BITBOOL_H__
#define _CMPH_BITBOOL_H__
#include "cmph_types.h" 
extern const cmph_uint8 bitmask[];


/** \def GETBIT(array, i)
 *  \brief get the value of an 1-bit integer stored in an array. 
 *  \param array to get 1-bit integer values from
 *  \param i is the index in array to get the 1-bit integer value from
 * 
 * GETBIT(array, i) is a macro that gets the value of an 1-bit integer stored in array.
 */
#define GETBIT(array, i) ((array[i >> 3] & bitmask[i & 0x00000007]) >> (i & 0x00000007))

/** \def SETBIT(array, i)
 *  \brief set 1 to an 1-bit integer stored in an array. 
 *  \param array to store 1-bit integer values
 *  \param i is the index in array to set the the bit to 1
 * 
 * SETBIT(array, i) is a macro that sets 1 to an 1-bit integer stored in an array.
 */
#define SETBIT(array, i) (array[i >> 3] |= bitmask[i & 0x00000007])

/** \def UNSETBIT(array, i)
 *  \brief set 0 to an 1-bit integer stored in an array. 
 *  \param array to store 1-bit integer values
 *  \param i is the index in array to set the the bit to 0
 * 
 * UNSETBIT(array, i) is a macro that sets 0 to an 1-bit integer stored in an array.
 */
#define UNSETBIT(array, i) (array[i >> 3] &= (~(bitmask[i & 0x00000007])))

//#define GETBIT(array, i) (array[(i) / 8] & bitmask[(i) % 8])
//#define SETBIT(array, i) (array[(i) / 8] |= bitmask[(i) % 8])
//#define UNSETBIT(array, i) (array[(i) / 8] &= (~(bitmask[(i) % 8])))

extern const cmph_uint8 valuemask[];

/** \def SETVALUE1(array, i, v)
 *  \brief set a value for a 2-bit integer stored in an array initialized with 1s. 
 *  \param array to store 2-bit integer values
 *  \param i is the index in array to set the value v
 *  \param v is the value to be set
 * 
 * SETVALUE1(array, i, v) is a macro that set a value for a 2-bit integer stored in an array.
 * The array should be initialized with all bits set to 1. For example:
 * memset(array, 0xff, arraySize);
 */
#define SETVALUE1(array, i, v) (array[i >> 2] &= ((v << ((i & 0x00000003) << 1)) | valuemask[i & 0x00000003]))

/** \def SETVALUE0(array, i, v)
 *  \brief set a value for a 2-bit integer stored in an array initialized with 0s. 
 *  \param array to store 2-bit integer values
 *  \param i is the index in array to set the value v
 *  \param v is the value to be set
 * 
 * SETVALUE0(array, i, v) is a macro that set a value for a 2-bit integer stored in an array.
 * The array should be initialized with all bits set to 0. For example:
 * memset(array, 0, arraySize);
 */
#define SETVALUE0(array, i, v) (array[i >> 2] |= (v << ((i & 0x00000003) << 1)))


/** \def GETVALUE(array, i)
 *  \brief get a value for a 2-bit integer stored in an array. 
 *  \param array to get 2-bit integer values from
 *  \param i is the index in array to get the value from
 * 
 * GETVALUE(array, i) is a macro that get a value for a 2-bit integer stored in an array.
 */
#define GETVALUE(array, i) ((array[i >> 2] >> ((i & 0x00000003) << 1)) & 0x00000003)


#endif
