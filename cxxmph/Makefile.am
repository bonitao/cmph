TESTS = $(check_PROGRAMS)
check_PROGRAMS = test_test seeded_hash_test mph_bits_test hollow_iterator_test mph_map_test mph_index_test trigraph_test map_tester_test string_util_test
noinst_PROGRAMS = bm_index bm_map
bin_PROGRAMS = cxxmph

cxxmph_includedir = $(includedir)/cxxmph/
cxxmph_include_HEADERS = mph_bits.h mph_map.h mph_index.h MurmurHash3.h trigraph.h seeded_hash.h stringpiece.h hollow_iterator.h string_util.h

check_LTLIBRARIES = libcxxmph_test.la
noinst_LTLIBRARIES = libcxxmph_bm.la
lib_LTLIBRARIES = libcxxmph.la
libcxxmph_la_SOURCES = MurmurHash3.cpp trigraph.cc mph_bits.cc mph_index.cc benchmark.h benchmark.cc string_util.cc
libcxxmph_la_LDFLAGS = -version-info 0:0:0
libcxxmph_test_la_SOURCES = test.h test.cc
libcxxmph_test_la_LIBADD = libcxxmph.la
libcxxmph_bm_la_SOURCES = benchmark.h benchmark.cc bm_common.h bm_common.cc
libcxxmph_bm_la_LIBADD = libcxxmph.la

test_test_SOURCES = test_test.cc
test_test_LDADD = libcxxmph_test.la $(CHECK_LIBS)

mph_map_test_LDADD = libcxxmph_test.la $(CHECK_LIBS)
mph_map_test_SOURCES = mph_map_test.cc

mph_index_test_LDADD   = libcxxmph.la
mph_index_test_SOURCES = mph_index_test.cc

trigraph_test_LDADD   = libcxxmph.la
trigraph_test_SOURCES = trigraph_test.cc

bm_index_LDADD = libcxxmph_bm.la -lcmph
bm_index_SOURCES = bm_index.cc 

bm_map_LDADD = libcxxmph_bm.la
bm_map_SOURCES = bm_map.cc

cxxmph_LDADD   = libcxxmph.la
cxxmph_SOURCES = cxxmph.cc

hollow_iterator_test_SOURCES = hollow_iterator_test.cc

seeded_hash_test_SOURCES = seeded_hash_test.cc
seeded_hash_test_LDADD   = libcxxmph.la

mph_bits_test_SOURCES = mph_bits_test.cc
mph_bits_test_LDADD   = libcxxmph.la

string_util_test_SOURCES = string_util_test.cc
string_util_test_LDADD   = libcxxmph.la

map_tester_test_SOURCES = map_tester.cc map_tester_test.cc
map_tester_test_LDADD = libcxxmph.la libcxxmph_test.la $(CHECK_LIBS)

