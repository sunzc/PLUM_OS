#ifndef __SBUNIX_H
#define __SBUNIX_H

#include <sys/defs.h>

extern uint64_t ticks;
extern void kbdinit(void);
extern void kbdintr(void);
void printf(const char *fmt, ...);
void put_to_screen(char c, int col, int row, int color);

static inline void outb(uint16_t port, uint8_t data) {
	__asm volatile("out %0, %1": : "a" (data), "d" (port));
}

static inline uint8_t inb(uint16_t port) {
	uint8_t data;

	__asm volatile("in %1, %0": "=a" (data): "d" (port));
	return data;
}

static inline void panic(char *msg) {
	printf("%s",msg);
	while(1);
}

#endif
