#include "uintstack.h"
#include <stdlib.h>

uintstack_t *uintstack_new(int capacity)
{
	uintstack_t *queue = (uintstack_t*)malloc(sizeof(uintstack_t));
	queue->capacity = capacity;
	queue->current = -1;
	queue->data = (uint32_t*)malloc(sizeof(uint32_t) * capacity);
	return queue;
}

int uintstack_pop(uintstack_t *queue, uint32_t *value)
{
	if(queue->current < 0)
		return 0;
	*value = queue->data[queue->current];
	queue->current--;
	return 1;
}

int uintstack_peek(uintstack_t *queue, uint32_t *value)
{
	if(queue->current < 0)
		return 0;
	*value = queue->data[queue->current];
	return 1;
}

int uintstack_push(uintstack_t *queue, uint32_t value)
{
	if((queue->current + 1) >= queue->capacity)
		return 0;
	queue->current++;
	queue->data[queue->current] = value;
	return 1;
}

void uintstack_destroy(uintstack_t *queue)
{
	free(queue->data);
	free(queue);
}