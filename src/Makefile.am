bin_PROGRAMS = cmph
lib_LTLIBRARIES = libcmph.la
include_HEADERS = cmph.h cmph_types.h
libcmph_la_SOURCES = debug.h\
				  bitbool.h bitbool.c\
				  cmph_types.h\
				  hash.h hash_state.h hash.c\
				  jenkins_hash.h jenkins_hash.c\
				  vstack.h vstack.c\
				  vqueue.h vqueue.c\
				  graph.h graph.c\
				  cmph.h cmph.c\
				  cmph_structs.h cmph_structs.c\
				  chm.h chm_structs.h chm.c\
				  bmz.h bmz_structs.h bmz.c\
				  bmz8.h bmz8_structs.h bmz8.c\
				  bdz.h bdz_structs.h bdz.c\
				  bdz_ph.h bdz_structs_ph.h bdz_ph.c\
				  buffer_manager.h buffer_manager.c\
				  buffer_entry.h buffer_entry.c\
				  brz.h brz_structs.h brz.c\
				  fch.h fch_structs.h fch.c\
				  fch_buckets.h fch_buckets.c

libcmph_la_LDFLAGS = -version-info 0:0:0

cmph_SOURCES = 	main.c wingetopt.h wingetopt.c
cmph_LDADD = libcmph.la
