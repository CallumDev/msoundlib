#ifndef _uintstack_H_
#define _uintstack_H_
#include <stdint.h>
typedef struct {
	int capacity;
	int current;
	uint32_t *data;
} uintstack_t;

uintstack_t *uintstack_new(int capacity);
int uintstack_pop(uintstack_t *queue, uint32_t *value);
int uintstack_peek(uintstack_t *queue, uint32_t *value);
int uintstack_push(uintstack_t *queue, uint32_t value);
void uintstack_destroy(uintstack_t *queue);
#endif