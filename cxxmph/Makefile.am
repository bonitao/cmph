bin_PROGRAMS = cmph_hash_map_test mphtable_test trigraph_test
lib_LTLIBRARIES = libcxxmph.la

libcxxmph_la_SOURCES = stringpiece.h MurmurHash2.h randomly_seeded_hash.h trigragh.h trigraph.cc mphtable.h mphtable.cc
libcxxmph_la_LDFLAGS = -version-info 0:0:0

cmph_hash_map_test_LDADD   = libcxxmph.la
cmph_hash_map_test_SOURCES = cmph_hash_map_test.cc

mphtable_test_LDADD   = libcxxmph.la
mphtable_test_SOURCES = mphtable_test.cc

trigraph_test_LDADD   = libcxxmph.la
trigraph_test_SOURCES = trigraph_test.cc
