#ifndef __BMZ_STRUCTS_H__
#define __BMZ_STRUCTS_H__

#include "hash_state.h"

struct cmph__bmz_mphf_data_t
{
	cmph_uint32 m; //edges (words) count
	cmph_uint32 n; //vertex count
	cmph_uint32 *g;
	cmph_hash_state_t **hashes;
};

struct cmph__bmz_mph_data_t
{
	CMPH_HASH hashfuncs[2];
	cmph_uint32 m; //edges (words) count
	cmph_uint32 n; //vertex count
	cmph_graph_t *graph;
	cmph_uint32 *g;
	cmph_hash_state_t **hashes;
};

#endif
