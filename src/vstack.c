#include "vstack.h"

#include <stdlib.h>
#include <assert.h>

//#define DEBUG
#include "debug.h"

struct cmph__vstack_t
{
	cmph_uint32 pointer;
	cmph_uint32 *values;
	cmph_uint32 capacity;
};

cmph_vstack_t *cmph_vstack_new()
{
	cmph_vstack_t *stack = (cmph_vstack_t *)malloc(sizeof(cmph_vstack_t));
	assert(stack);
	stack->pointer = 0;
	stack->values = NULL;
	stack->capacity = 0;
	return stack;
}

void cmph_vstack_destroy(cmph_vstack_t *stack)
{
	assert(stack);
	free(stack->values);
	free(stack);
}

void cmph_vstack_push(cmph_vstack_t *stack, cmph_uint32 val)
{
	assert(stack);
	cmph_vstack_reserve(stack, stack->pointer + 1);
	stack->values[stack->pointer] = val;
	++(stack->pointer);
}
void cmph_vstack_pop(cmph_vstack_t *stack)
{
	assert(stack);
	assert(stack->pointer > 0);
	--(stack->pointer);
}

cmph_uint32 cmph_vstack_top(cmph_vstack_t *stack)
{
	assert(stack);
	assert(stack->pointer > 0);
	return stack->values[(stack->pointer - 1)];
}
int cmph_vstack_empty(cmph_vstack_t *stack)
{
	assert(stack);
	return stack->pointer == 0;
}
cmph_uint32 cmph_vstack_size(cmph_vstack_t *stack)
{
	return stack->pointer;
}
void cmph_vstack_reserve(cmph_vstack_t *stack, cmph_uint32 size)
{
	assert(stack);
	if (stack->capacity < size)
	{
		cmph_uint32 new_capacity = stack->capacity + 1;
		DEBUGP("Increasing current capacity %u to %u\n", stack->capacity, size);
		while (new_capacity	< size)
		{
			new_capacity *= 2;
		}
		stack->values = (cmph_uint32 *)realloc(stack->values, sizeof(cmph_uint32)*new_capacity);
		assert(stack->values);
		stack->capacity = new_capacity;
		DEBUGP("Increased\n");
	}
}
		
