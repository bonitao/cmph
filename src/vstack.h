#ifndef __CMPH_VSTACK_H__
#define __CMPH_VSTACK_H__

#include "cmph_types.h"
typedef struct cmph__vstack_t cmph_vstack_t;

cmph_vstack_t *cmph_vstack_new();
void cmph_vstack_destroy(cmph_vstack_t *stack);

void cmph_vstack_push(cmph_vstack_t *stack, cmph_uint32 val);
cmph_uint32 cmph_vstack_top(cmph_vstack_t *stack);
void cmph_vstack_pop(cmph_vstack_t *stack);
int cmph_vstack_empty(cmph_vstack_t *stack);
cmph_uint32 cmph_vstack_size(cmph_vstack_t *stack);

void cmph_vstack_reserve(cmph_vstack_t *stack, cmph_uint32 size);

#endif
