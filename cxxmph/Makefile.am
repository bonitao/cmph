AM_CXXFLAGS='-std=c++0x'
TESTS = $(check_PROGRAMS)
check_PROGRAMS = mph_map_test mph_table_test trigraph_test
noinst_PROGRAMS = bm_numbers bm_urls
bin_PROGRAMS = cxxmph
lib_LTLIBRARIES = libcxxmph.la
libcxxmph_la_SOURCES = MurmurHash2.h trigragh.h trigraph.cc mph_table.h mph_table.cc seeded_hash.h stringpiece.h benchmark.h benchmark.cc
libcxxmph_la_LDFLAGS = -version-info 0:0:0
cxxmph_includedir = $(includedir)/cxxmph/
cxxmph_include_HEADERS = mph_map.h mph_table.h MurmurHash2.h trigraph.h seeded_hash.h stringpiece.h

mph_map_test_LDADD   = libcxxmph.la
mph_map_test_SOURCES = mph_map_test.cc

mph_table_test_LDADD   = libcxxmph.la
mph_table_test_SOURCES = mph_table_test.cc

trigraph_test_LDADD   = libcxxmph.la
trigraph_test_SOURCES = trigraph_test.cc

bm_numbers_LDADD = libcxxmph.la
bm_numbers_SOURCES = bm_numbers.cc

bm_urls_LDADD = libcxxmph.la
bm_urls_SOURCES = bm_urls.cc

cxxmph_LDADD   = libcxxmph.la
cxxmph_SOURCES = cxxmph.cc
