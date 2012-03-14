#ifndef __CXXMPH_MPH_BITS_H__
#define __CXXMPH_MPH_BITS_H__

#include <stdint.h>  // for uint32_t and friends
#include <climits>

namespace cxxmph {

static const uint8_t valuemask[] = { 0xfc, 0xf3, 0xcf, 0x3f};
static void set_2bit_value(uint8_t *d, uint32_t i, uint8_t v) {
  d[(i >> 2)] &= ((v << ((i & 3) << 1)) | valuemask[i & 3]);
}
static uint32_t get_2bit_value(const uint8_t* d, uint32_t i) {
  return (d[(i >> 2)] >> (((i & 3) << 1)) & 3);
}
static uint32_t nextpoweroftwo(uint32_t k) {
  if (k == 0) return 1;
  k--;
  for (int i=1; i<sizeof(uint32_t)*CHAR_BIT; i<<=1) k = k | k >> i;
  return k+1;
}
  
}  // namespace cxxmph

#endif
