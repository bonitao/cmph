#include "vqueue.h"
#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
struct __vqueue_t
{
  uint32 * values;
  uint32 beg, end, capacity;
};

vqueue_t * vqueue_new(uint32 capacity)
{
  vqueue_t *q = (vqueue_t *)malloc(sizeof(vqueue_t));
  assert(q);
  q->values = (uint32 *)calloc(capacity+1, sizeof(uint32));
  q->beg = q->end = 0;
  q->capacity = capacity+1;
  return q;
}

uint8 vqueue_is_empty(vqueue_t * q)
{
  return (q->beg == q->end);
}

void vqueue_insert(vqueue_t * q, uint32 val)
{
  assert((q->end + 1)%q->capacity != q->beg); // Is queue full?
  q->end = (q->end + 1)%q->capacity;
  q->values[q->end] = val;
}

uint32 vqueue_remove(vqueue_t * q)
{
  assert(!vqueue_is_empty(q)); // Is queue empty?
  q->beg = (q->beg + 1)%q->capacity;
  return q->values[q->beg];
}

void vqueue_print(vqueue_t * q)
{
  uint32 i;
  for (i = q->beg; i != q->end; i = (i + 1)%q->capacity)
    fprintf(stderr, "%u\n", q->values[(i + 1)%q->capacity]);
} 

void vqueue_destroy(vqueue_t *q)
{
  free(q->values); q->values = NULL;
}
