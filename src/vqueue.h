#ifndef __CMPH_VQUEUE_H__
#define __CMPH_VQUEUE_H__

#include "cmph_types.h"
typedef struct cmph__vqueue_t cmph_vqueue_t;

cmph_vqueue_t * cmph_vqueue_new(cmph_uint32 capacity);

cmph_uint8 cmph_vqueue_is_empty(cmph_vqueue_t * q);

void cmph_vqueue_insert(cmph_vqueue_t * q, cmph_uint32 val);

cmph_uint32 cmph_vqueue_remove(cmph_vqueue_t * q);

void cmph_vqueue_print(cmph_vqueue_t * q);

void cmph_vqueue_destroy(cmph_vqueue_t * q);
#endif
