#ifndef __CMPH_VQUEUE_H__
#define __CMPH_VQUEUE_H__

#include "cmph_types.h"
typedef struct __vqueue_t vqueue_t;

vqueue_t * vqueue_new(uint32 capacity);

uint8 vqueue_is_empty(vqueue_t * q);

void vqueue_insert(vqueue_t * q, uint32 val);

uint32 vqueue_remove(vqueue_t * q);

void vqueue_print(vqueue_t * q);

void vqueue_destroy(vqueue_t * q);
#endif
