#include "vqueue.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
struct cmph__vqueue_t
{
  cmph_uint32 * values;
  cmph_uint32 beg, end, capacity;
};

cmph_vqueue_t * cmph_vqueue_new(cmph_uint32 capacity)
{
  cmph_vqueue_t *q = (cmph_vqueue_t *)malloc(sizeof(cmph_vqueue_t));
  assert(q);
  q->values = (cmph_uint32 *)calloc(capacity+1, sizeof(cmph_uint32));
  q->beg = q->end = 0;
  q->capacity = capacity+1;
  return q;
}

cmph_uint8 cmph_vqueue_is_empty(cmph_vqueue_t * q)
{
  return (q->beg == q->end);
}

void cmph_vqueue_insert(cmph_vqueue_t * q, cmph_uint32 val)
{
  assert((q->end + 1)%q->capacity != q->beg); // Is queue full?
  q->end = (q->end + 1)%q->capacity;
  q->values[q->end] = val;
}

cmph_uint32 cmph_vqueue_remove(cmph_vqueue_t * q)
{
  assert(!cmph_vqueue_is_empty(q)); // Is queue empty?
  q->beg = (q->beg + 1)%q->capacity;
  return q->values[q->beg];
}

void cmph_vqueue_print(cmph_vqueue_t * q)
{
  cmph_uint32 i;
  for (i = q->beg; i != q->end; i = (i + 1)%q->capacity)
    fprintf(stderr, "%u\n", q->values[(i + 1)%q->capacity]);
} 

void cmph_vqueue_destroy(cmph_vqueue_t *q)
{
  free(q->values); q->values = NULL;
}
