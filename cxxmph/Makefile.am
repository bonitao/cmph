TESTS = $(check_PROGRAMS)
check_PROGRAMS = seeded_hash_test mph_bits_test hollow_iterator_test mph_map_test mph_index_test trigraph_test perfect_cuckoo_cache_line_test perfect_cuckoo_test
noinst_PROGRAMS = bm_index bm_map
bin_PROGRAMS = cxxmph

cxxmph_includedir = $(includedir)/cxxmph/
cxxmph_include_HEADERS = mph_bits.h mph_map.h mph_index.h MurmurHash3.h trigraph.h seeded_hash.h stringpiece.h hollow_iterator.h string_util.h

noinst_LTLIBRARIES = libcxxmph_bm.la
lib_LTLIBRARIES = libcxxmph.la
libcxxmph_la_SOURCES = MurmurHash3.h MurmurHash3.cpp trigragh.h trigraph.cc mph_index.h mph_index.cc seeded_hash.h stringpiece.h benchmark.h benchmark.cc mph_bits.h mph_bits.cc
libcxxmph_la_LDFLAGS = -version-info 0:0:0
libcxxmph_test_la_SOURCES = test.h test.cc
libcxxmph_test_la_LIBADD = libcxxmph.la
libcxxmph_bm_la_SOURCES = benchmark.h benchmark.cc bm_common.h bm_common.cc
libcxxmph_bm_la_LIBADD = libcxxmph.la

test_test_SOURCES = test_test.cc
test_test_LDADD = libcxxmph_test.la $(CHECK_LIBS)

mph_map_test_LDADD = libcxxmph_test.la $(CHECK_LIBS)
mph_map_test_SOURCES = mph_map_test.cc
dense_hash_map_test_LDADD = libcxxmph_test.la $(CHECK_LIBS)
dense_hash_map_test_SOURCES = dense_hash_map_test.cc

mph_index_test_LDADD   = libcxxmph.la
mph_index_test_SOURCES = mph_index_test.cc

trigraph_test_LDADD   = libcxxmph.la
trigraph_test_SOURCES = trigraph_test.cc

# Bad dependency, do not compile by default.
# bm_index_LDADD = libcxxmph_bm.la -lcmph
# bm_index_SOURCES = bm_index.cc 

bm_map_LDADD = libcxxmph_bm.la
bm_map_SOURCES = bm_map.cc

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
