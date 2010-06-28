#include <numerical_limits>

template <int n, int mask = 1 << 7> struct bitcount {
  enum { value = (n & mask ? 1:0) + bitcount<n, mask >> 1>::value };
};
template <int n> struct bitcount<n, 0> { enum { value = 0 }; };

template <int n, int current, int mask = 1 << 8> struct bitposition {
  enum 

template <int index = 0, typename op> class CompileTimeByteTable {
 public:
  CompileTimeByteTable : current(op<index>::value) { }
  int operator[] (int i) { return *(&current + i); }
private:
  unsigned char current;
  CompileTimeByteTable<next> next;
};

static CompileTimeByteTable<256, bitcount> BitcountTable;


#define mix(a,b,c) \
{ \
        a -= b; a -= c; a ^= (c>>13); \
        b -= c; b -= a; b ^= (a<<8); \
        c -= a; c -= b; c ^= (b>>13); \
        a -= b; a -= c; a ^= (c>>12);  \
        b -= c; b -= a; b ^= (a<<16); \
        c -= a; c -= b; c ^= (b>>5); \
        a -= b; a -= c; a ^= (c>>3);  \
        b -= c; b -= a; b ^= (a<<10); \
        c -= a; c -= b; c ^= (b>>15); \
}


static const int kMaskStepSelectTable = std::limit<char>::max;
