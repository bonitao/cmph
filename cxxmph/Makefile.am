check_PROGRAMS = cmph_hash_map_test mphtable_test trigraph_test
noinst_PROGRAMS = bm_numbers bm_urls
bin_PROGRAMS = cxxmph
lib_LTLIBRARIES = libcxxmph.la
include_HEADERS = cmph_hash_map.h mphtable.h MurmurHash2.h trigraph.h cxxmph_hash.h stringpiece.h

libcxxmph_la_SOURCES = MurmurHash2.h trigragh.h trigraph.cc mphtable.h mphtable.cc cxxmph_hash.h stringpiece.h benchmark.h benchmark.cc
libcxxmph_la_LDFLAGS = -version-info 0:0:0

cmph_hash_map_test_LDADD   = libcxxmph.la
cmph_hash_map_test_SOURCES = cmph_hash_map_test.cc

mphtable_test_LDADD   = libcxxmph.la
mphtable_test_SOURCES = mphtable_test.cc

trigraph_test_LDADD   = libcxxmph.la
trigraph_test_SOURCES = trigraph_test.cc

bm_numbers_LDADD = libcxxmph.la
bm_numbers_SOURCES = bm_numbers.cc

bm_urls_LDADD = libcxxmph.la
bm_urls_SOURCES = bm_urls.cc

cxxmph_LDADD   = libcxxmph.la
cxxmph_SOURCES = cxxmph.cc
