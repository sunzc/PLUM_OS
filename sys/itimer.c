#include <sys/sbunix.h>
#include <sys/irq.h>
#include <sys/pic.h>

#define IO_TIMER1	0x40	// 8253 Timer #1

#define TIMER_FREQ	1193182
#define TIMER_DIV(x)	((TIMER_FREQ + (x)/2)/(x))

#define TIMER_MODE	(IO_TIMER1 + 3) // timer mode port
#define TIMER_SEL0	0x00	// select counter 0
#define TIMER_RATEGEN	0x04	// mode 2, rate generator
#define TIMER_16BIT	0x30	// r/w counter 16 bits, LSB first

void timerinit(void) {
	// Interrupt 100 times/sec
	outb(TIMER_MODE, TIMER_SEL0 | TIMER_RATEGEN | TIMER_16BIT);
	outb(IO_TIMER1, TIMER_DIV(100) % 256);
	outb(IO_TIMER1, TIMER_DIV(100) / 256);
	picenable(IRQ_TIMER);
}
