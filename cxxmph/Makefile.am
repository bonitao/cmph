TESTS = $(check_PROGRAMS)
check_PROGRAMS = seeded_hash_test mph_bits_test hollow_iterator_test mph_map_test mph_index_test trigraph_test perfect_cuckoo_cache_line_test perfect_cuckoo_test
noinst_PROGRAMS = bm_index bm_map
bin_PROGRAMS = cxxmph
lib_LTLIBRARIES = libcxxmph.la
libcxxmph_la_SOURCES = MurmurHash3.h MurmurHash3.cpp trigragh.h trigraph.cc mph_index.h mph_index.cc seeded_hash.h stringpiece.h benchmark.h benchmark.cc mph_bits.h mph_bits.cc
libcxxmph_la_LDFLAGS = -version-info 0:0:0
cxxmph_includedir = $(includedir)/cxxmph/
cxxmph_include_HEADERS = mph_map.h mph_index.h MurmurHash3.h trigraph.h seeded_hash.h stringpiece.h hollow_iterator.h

mph_map_test_LDADD   = libcxxmph.la
mph_map_test_SOURCES = mph_map_test.cc

mph_index_test_LDADD   = libcxxmph.la
mph_index_test_SOURCES = mph_index_test.cc

bm_index_LDADD = libcxxmph.la -lcmph
bm_index_SOURCES = bm_common.cc bm_index.cc 

trigraph_test_LDADD   = libcxxmph.la
trigraph_test_SOURCES = trigraph_test.cc

bm_map_LDADD = libcxxmph.la
bm_map_SOURCES = bm_common.cc bm_map.cc

cxxmph_LDADD   = libcxxmph.la
cxxmph_SOURCES = cxxmph.cc

hollow_iterator_test_SOURCES = hollow_iterator_test.cc

seeded_hash_test_SOURCES = seeded_hash_test.cc
seeded_hash_test_LDADD   = libcxxmph.la

mph_bits_test_SOURCES = mph_bits_test.cc
mph_bits_test_LDADD   = libcxxmph.la

perfect_cuckoo_cache_line_test_SOURCES = perfect_cuckoo_cache_line_test.cc
perfect_cuckoo_cache_line_test_LDADD   = libcxxmph.la

perfect_cuckoo_test_SOURCES = perfect_cuckoo_test.cc
perfect_cuckoo_test_LDADD   = libcxxmph.la
