#include <sys/sbunix.h>
#include <sys/irq.h>
#include <sys/kbd.h>
#include <sys/pic.h>
#include <sys/proc.h>

#define KBD_STAT_PORT		0x64	// kbd controller status port(I)
#define KBD_DATA_INBF		0x01	// kdb data in buffer
#define KBD_DATA_PORT		0x60	// kbd data port(I)

/* we implement terminal here, for it's convenient */

#define STDIN_BUF_SIZE		4096
char *stdin_buf;
int stdin_buf_size = 0;

/* state 0: not ready, 1 ready */
int stdin_buf_state = 0;

/* if process wait for stdin, it will sleep here */
extern task_struct *wait_stdin_list;
extern task_struct *current;

void kbdinit(void) {
	stdin_buf = (char *)get_zero_page();
	picenable(IRQ_KBD);
}

static int is_normal_key_pressed(uint16_t data);
static int is_special_key_pressed(uint16_t data);
static int is_normal_key_released(uint16_t data);
static int is_special_key_released(uint16_t data);

static uint8_t normal_key_pressed = 0;
static uint8_t special_key_pressed = 0;

void kbdintr(void) {
	uint16_t status, data;
	task_struct *p;
	
	status = inb(KBD_STAT_PORT);
	if((status & KBD_DATA_INBF) == 0) {
		printf("in kbdintr: st wrong\n");
		return;
	}
	data = inb(KBD_DATA_PORT);

////////////////	printf("key pressed:0x%x\n",data);
	
	if(is_normal_key_pressed(data))
		normal_key_pressed = data;
	else if (is_special_key_pressed(data))
		special_key_pressed = data;
	else if (is_normal_key_released(data) && special_key_pressed == 0) {
		if(normal_key_pressed != 0xe) {	// backspace
			print_to_screen(normalmap[normal_key_pressed]);
//			if(normal_key_pressed != 0x1c)
//				print_to_screen(0x87);
			stdin_buf[stdin_buf_size++] = normalmap[normal_key_pressed];
			if(normal_key_pressed == 0x1c) {	// '\n' 0x9c 0x39 ' '
				stdin_buf_state = 1; //BUF_READY
				if (wait_stdin_list != NULL)
					wakeup(&wait_stdin_list, NULL, 0);
			}
		} else {
			print_to_screen(0xe);
			//print_to_screen(0x87);
			stdin_buf_size--;
		}
		normal_key_pressed = 0; // clear it
	} else if (is_normal_key_released(data) && special_key_pressed != 0) {
		switch(special_key_pressed) { 
			// to make life easier, we handle special key when either normal key or special key
			// released
			case 0x2A: // Shift
				print_to_screen(shiftmap[normal_key_pressed]);
				stdin_buf[stdin_buf_size++] = shiftmap[normal_key_pressed];
				break;
			case 0x38: // Alt
				put_to_screen('@', 64, 24, 0);
				put_to_screen(normalmap[normal_key_pressed], 65, 24, 0);
				break;
			case 0x1D: // Ctrl
				put_to_screen('^', 64, 24, 0);
				put_to_screen(shiftmap[normal_key_pressed], 65, 24, 0);
				if (normal_key_pressed == 0x2e) { //Ctrl^C, kill user-process
					if(wait_stdin_list != NULL) {
						p = wait_stdin_list;
						while(p->next != NULL)
							p = p->next;
						kill_by_pid(p->pid, SIGKILL);
					} else if(current->mm != NULL)
						kill_by_pid(current->pid, SIGKILL);
					// else do nothing
				}
				if (normal_key_pressed == 0x26) // Ctrl-L
					clear_screen();
				break;
		}

		normal_key_pressed = 0;
		special_key_pressed = 0;

	} else if (is_special_key_released(data)) {
		if (normal_key_pressed) { // clear normal key too, we handle it when both are pressed
			switch(special_key_pressed) { 
				// to make life easier, we handle special key when either normal key or special key
				// released
				case 0x2A: // Shift
					print_to_screen(shiftmap[normal_key_pressed]);
					stdin_buf[stdin_buf_size++] = shiftmap[normal_key_pressed];
					break;
				case 0x38: // Alt
					put_to_screen('@', 64, 24, 0);
					put_to_screen(normalmap[normal_key_pressed], 65, 24, 0);
					break;
				case 0x1D: // Ctrl
					put_to_screen('^', 64, 24, 0);
					put_to_screen(shiftmap[normal_key_pressed], 65, 24, 0);
					if (normal_key_pressed == 0x2e) { //Ctrl^C, kill user-process
						if(wait_stdin_list != NULL) {
							p = wait_stdin_list;
							while(p->next != NULL)
								p = p->next;
							kill_by_pid(p->pid, SIGKILL);
						} else if(current->mm != NULL)
							kill_by_pid(current->pid, SIGKILL);
						// else do nothing
					}
					if (normal_key_pressed == 0x26) // Ctrl-L
						clear_screen();
					break;
			}

			normal_key_pressed = 0; // clear it
		}

		special_key_pressed = 0; // clear it
	}

	pic_sendeoi(33);

	return;
}

static int is_normal_key_pressed(uint16_t data) {
	if (normalmap[data] != 0) // B pressed
		return 1;
	else
		return 0;
}

static int is_normal_key_released(uint16_t data) {
	if (normalmap[data - 0x80] != 0) // B released, 0x32 is followed, but escape it first
		return 1;
	else
		return 0;
}

static int is_special_key_released(uint16_t data) {
	if (data == 0x9D || data == 0xAA || data == 0xB8) // Left Ctrl released, 0x14 is followed, but escape it first
		return 1;
	else
		return 0;
}

static int is_special_key_pressed(uint16_t data) {
	if (data == 0x1D || data == 0x2A || data == 0x38) // Left Ctrl pressed
		return 1;
	else
		return 0;
}
