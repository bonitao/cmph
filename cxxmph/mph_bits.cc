#include "mph_bits.h"

namespace cxxmph {

const uint8_t dynamic_2bitset::vmask[] = { 0xfc, 0xf3, 0xcf, 0x3f};

template <int n, int mask = (1 << 7)> struct bitcount {
enum { value = (n & mask ? 1:0) + bitcount<n, (mask >> 1)>::value };
};
template <int n> struct bitcount<n, 0> { enum { value = 0 }; };

template <int size, int index = size>
class CompileTimeRankTable {
public:
CompileTimeRankTable() : current(bitcount<index - 1>::value) { }
uint8_t operator[] (uint8_t i) { return *(&current + size - i - 1); }
private:
unsigned char current;
CompileTimeRankTable<index -1> next;
};
template <int size> class CompileTimeRankTable<size, 0> { };
static CompileTimeRankTable<256> kRanktable;

uint8_t Ranktable::get(uint8_t i) { return kRanktable[i]; }



}
