#include <sys/sbunix.h>
#include <sys/pic.h>

static int sec = 0;
static int min = 0;
static int hour = 0;

static void write_time_to_screen (int h, int m, int s);

void interrupt_handler_timer(void) {
	ticks++;	
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
	pic_sendeoi(32);
	return;
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
