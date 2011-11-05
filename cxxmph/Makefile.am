TESTS = $(check_PROGRAMS)
check_PROGRAMS = mph_map_test mph_index_test
noinst_PROGRAMS = bm_index bm_map
bin_PROGRAMS = cxxmph
lib_LTLIBRARIES = libcxxmph.la
libcxxmph_la_SOURCES = MurmurHash2.h trigragh.h trigraph.cc mph_index.h mph_index.cc seeded_hash.h stringpiece.h benchmark.h benchmark.cc
libcxxmph_la_LDFLAGS = -version-info 0:0:0
cxxmph_includedir = $(includedir)/cxxmph/
cxxmph_include_HEADERS = mph_map.h mph_index.h MurmurHash2.h trigraph.h seeded_hash.h stringpiece.h

mph_map_test_LDADD   = libcxxmph.la
mph_map_test_SOURCES = mph_map_test.cc

mph_index_test_LDADD   = libcxxmph.la
mph_index_test_SOURCES = mph_index_test.cc

bm_index_LDADD = libcxxmph.la
bm_index_SOURCES = bm_common.cc bm_index.cc 

bm_map_LDADD = libcxxmph.la
bm_map_SOURCES = bm_common.cc bm_map.cc

cxxmph_LDADD   = libcxxmph.la
cxxmph_SOURCES = cxxmph.cc
