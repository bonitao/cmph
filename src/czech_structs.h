#ifndef __CMPH_CZECH_STRUCTS_H__
#define __CMPH_CZECH_STRUCTS_H__

#include "hash_state.h"

struct __czech_data_t
{
	cmph_uint32 m; //edges (words) count
	cmph_uint32 n; //vertex count
	cmph_uint32 *g;
	hash_state_t **hashes;
};

struct __czech_config_data_t
{
	CMPH_HASH hashfuncs[2];
	cmph_uint32 m; //edges (words) count
	cmph_uint32 n; //vertex count
	graph_t *graph;
	cmph_uint32 *g;
	hash_state_t **hashes;
};

#endif
