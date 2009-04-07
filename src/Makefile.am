bin_PROGRAMS = cmph
lib_LTLIBRARIES = libcmph.la
include_HEADERS = cmph.h cmph_types.h
libcmph_la_SOURCES =  hash.c jenkins_hash.c\
		      vstack.c vqueue.c\
		      graph.c cmph.c  cmph_structs.c\
		      chm.c bmz.c bmz8.c bdz.c bdz_ph.c\
		      buffer_manager.c buffer_entry.c\
		      brz.c fch.c fch_buckets.c \
		      select.c compressed_seq.c \
		      chd.c chd_ph.c miller_rabin.c \
		      compressed_rank.c 

libcmph_la_LDFLAGS = -version-info 0:0:0

cmph_SOURCES = 	main.c wingetopt.h wingetopt.c
cmph_LDADD = libcmph.la
