#ifndef __BMZ_STRUCTS_H__
#define __BMZ_STRUCTS_H__

#include "hash_state.h"

struct __bmz_mphf_data_t
{
	uint32 m; //edges (words) count
	uint32 n; //vertex count
	uint32 *g;
	hash_state_t **hashes;
};

struct __bmz_mph_data_t
{
	CMPH_HASH hashfuncs[2];
	uint32 m; //edges (words) count
	uint32 n; //vertex count
	graph_t *graph;
	uint32 *g;
	hash_state_t **hashes;
};

#endif
