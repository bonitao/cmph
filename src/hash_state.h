#ifndef __HASH_STATE_H__
#define __HASH_STATE_H__

#include "hash.h"
#include "jenkins_hash.h"
#include "djb2_hash.h"
#include "sdbm_hash.h"
#include "fnv_hash.h"
union __hash_state_t
{
	CMPH_HASH hashfunc;
	jenkins_state_t jenkins;
	djb2_state_t djb2;
	sdbm_state_t sdbm;
	fnv_state_t fnv;
};

#endif
