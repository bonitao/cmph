#ifndef __HASH_STATE_H__
#define __HASH_STATE_H__

#include "hash.h"
#include "jenkins_hash.h"
#include "djb2_hash.h"
#include "sdbm_hash.h"
#include "fnv_hash.h"
union cmph__hash_state_t
{
	CMPH_HASH hashfunc;
	cmph_jenkins_state_t jenkins;
	cmph_djb2_state_t djb2;
	cmph_sdbm_state_t sdbm;
	cmph_fnv_state_t fnv;
};

#endif
