#include <sys/sbunix.h>
#include <sys/proc.h>
#include <sys/pic.h>

extern task_struct *current;

#define DISPLAY_TIME

static int sec = 0;
static int min = 0;
static int hour = 0;

static void write_time_to_screen (int h, int m, int s);

void interrupt_handler_timer(void) {

	ticks++;

#ifdef	DISPLAY_TIME
	if (ticks % 100 == 0) {
		sec++;
		if (sec % 60 == 0) {
			sec = 0;
			min++;
			if (min % 60 == 0) {
				min = 0;
				hour++;
			}
		}
		//printf(" %d: %d: %d\n", hour, min, sec);
		write_time_to_screen(hour, min, sec);
	}
#endif

	if (current->timeslice == 10) {
		current->timeslice = 0;
		pic_sendeoi(32);
		schedule();
	} else {
		current->timeslice += 1;
		pic_sendeoi(32);
	}
}

static void write_time_to_screen (int h, int m, int s) {
	if (h >= 10)
		put_to_screen(h/10 + '0',70,24,9);
	else
		put_to_screen('0',70,24,9);
	put_to_screen(h%10 + '0',71,24,9);
	put_to_screen(':',72,24,9);
	if (m >= 10)
		put_to_screen(m/10 + '0',73,24,9);
	else
		put_to_screen('0',73,24,9);
	put_to_screen(m%10 + '0',74,24,9);
	put_to_screen(':',75,24,9);
	if (s >= 10)
		put_to_screen(s/10 + '0',76,24,9);
	else
		put_to_screen('0',76,24,9);
	put_to_screen(s%10 + '0',77,24,9);
}
