#include <sys/sbunix.h>

void interrupt_handler_timer(void) {
	ticks++;	
	printf("ticks : %lx\n", ticks);
	return;
}
