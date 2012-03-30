#ifndef ranksel_macros_h
#define ranksel_macros_h

#define ONES_STEP_4 ( 0x1111111111111111ULL )
#define ONES_STEP_8 ( 0x0101010101010101ULL )
#define ONES_STEP_9 ( 1ULL << 0 | 1ULL << 9 | 1ULL << 18 | 1ULL << 27 | 1ULL << 36 | 1ULL << 45 | 1ULL << 54 )
#define ONES_STEP_16 ( 1ULL << 0 | 1ULL << 16 | 1ULL << 32 | 1ULL << 48 )
#define MSBS_STEP_4 ( 0x8ULL * ONES_STEP_4 )
#define MSBS_STEP_8 ( 0x80ULL * ONES_STEP_8 )
#define MSBS_STEP_9 ( 0x100ULL * ONES_STEP_9 )
#define MSBS_STEP_16 ( 0x8000ULL * ONES_STEP_16 )
#define INCR_STEP_8 ( 0x80ULL << 56 | 0x40ULL << 48 | 0x20ULL << 40 | 0x10ULL << 32 | 0x8ULL << 24 | 0x4ULL << 16 | 0x2ULL << 8 | 0x1 )

#define ONES_STEP_32 ( 0x0000000100000001ULL )
#define MSBS_STEP_32 ( 0x8000000080000000ULL )
	
#define COMPARE_STEP_8(x,y) ( ( ( ( ( (x) | MSBS_STEP_8 ) - ( (y) & ~MSBS_STEP_8 ) ) ^ (x) ^ ~(y) ) & MSBS_STEP_8 ) >> 7 )
#define LEQ_STEP_8(x,y) ( ( ( ( ( (y) | MSBS_STEP_8 ) - ( (x) & ~MSBS_STEP_8 ) ) ^ (x) ^ (y) ) & MSBS_STEP_8 ) >> 7 )

#define UCOMPARE_STEP_9(x,y) ( ( ( ( ( ( (x) | MSBS_STEP_9 ) - ( (y) & ~MSBS_STEP_9 ) ) | ( x ^ y ) ) ^ ( x | ~y ) ) & MSBS_STEP_9 ) >> 8 )
#define UCOMPARE_STEP_16(x,y) ( ( ( ( ( ( (x) | MSBS_STEP_16 ) - ( (y) & ~MSBS_STEP_16 ) ) | ( x ^ y ) ) ^ ( x | ~y ) ) & MSBS_STEP_16 ) >> 15 )
#define ULEQ_STEP_9(x,y) ( ( ( ( ( ( (y) | MSBS_STEP_9 ) - ( (x) & ~MSBS_STEP_9 ) ) | ( x ^ y ) ) ^ ( x & ~y ) ) & MSBS_STEP_9 ) >> 8 )
#define ULEQ_STEP_16(x,y) ( ( ( ( ( ( (y) | MSBS_STEP_16 ) - ( (x) & ~MSBS_STEP_16 ) ) | ( x ^ y ) ) ^ ( x & ~y ) ) & MSBS_STEP_16 ) >> 15 )
#define ZCOMPARE_STEP_8(x) ( ( ( x | ( ( x | MSBS_STEP_8 ) - ONES_STEP_8 ) ) & MSBS_STEP_8 ) >> 7 )

#endif
