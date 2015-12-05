#include <sys/sbunix.h>
#include <stdarg.h>

#define MAX_BUF_SIZE 512 
#define NUM_BUF 80

#define MAX_COLUMN 	80
//#define MAX_ROW	25
#define MAX_ROW		24

#define VGA_BUFFER_ADDR 0xffffffff800b8000;

typedef unsigned long uint64_t;
typedef uint64_t size_t;

extern int stdin_buf_size;

static char *int_to_string(char *buf, int n);
static char *longlong_to_hexstring(char *buf, unsigned long long n);
static char *longlong_to_string(char *buf, unsigned long long n);
static void scoll_screen_up(void);
void put_to_screen(char c, int col, int row, int color);
void clear_screen(void);
int vsprintf(char *str, const char *fmt, va_list ap);

typedef struct loc{
	int row;
	int col;
}loc;

loc cursor = {
		.col = 0,
		.row = 0,
	     };

static char* screen_buffer = (char *)VGA_BUFFER_ADDR;

void put_to_screen(char c, int col, int row, int color){
	screen_buffer[row * MAX_COLUMN * 2 + col * 2] = c;
	screen_buffer[row * MAX_COLUMN * 2 + col * 2 + 1] = color;
}

void printf(const char *format, ...) {
	va_list val;
	char str[MAX_BUF_SIZE] = {0};
	int printed = 0;

	va_start(val, format);
	vsprintf(str, format, val);
	va_end(val);

	while(str[printed]) {
		print_to_screen(str[printed]);
		++printed;
	}

	return;
}

int vsprintf(char *str, const char *fmt, va_list ap){
	char c;
	int n;
	unsigned long long ull;
	char uc;
	const char *s;
	size_t idx = 0;
	size_t j;
	char buf[NUM_BUF];

	for(;;) {
		while((c = *fmt++) != 0){
			if(c == '%')
				break;
			str[idx++] = c;
		}

		if(c == 0)
			break;

		c = *fmt++;
		if(c == 0)
			break;

		/* we do a naive parse, only %d, %s, %c, %lx is accept */
		if(c == 'c'){
			uc = va_arg(ap, unsigned int);
			str[idx++] = uc;
		} else if(c == 's'){
			s = va_arg(ap, const char *);
			if(s == 0)
				s = "<null>";
			j = 0;
			while(s[j] != 0){
				str[idx++] = s[j++];
			}
		} else if(c == 'd'){
			n = va_arg(ap, int);
			s = int_to_string(buf, n);
			j = 0;
			while(s[j] != 0){
				str[idx++] = s[j++];
			}
		} else if(c == 'x'){
			ull = va_arg(ap, uint64_t);
			s = longlong_to_hexstring(buf, ull);
			j = 0;
			while(s[j] != 0){
				str[idx++] = s[j++];
			}
		} else if(c == 'p'){
			str[idx++] = '0';
			str[idx++] = 'x';
			ull = va_arg(ap, uint64_t);
			s = longlong_to_hexstring(buf, ull);
			j = 0;
			while(s[j] != 0){
				str[idx++] = s[j++];
			}
		} else if(c == 'l'){
			c = *fmt++;
			if(c == 'x'){
				ull = va_arg(ap, unsigned long long);
				s = longlong_to_hexstring(buf, ull);
				j = 0;
				while(s[j] != 0){
					str[idx++] = s[j++];
				}
			} else if(c == 'u'){
				ull = va_arg(ap, unsigned long long);
				s = longlong_to_string(buf, ull);
				j = 0;
				while(s[j] != 0){
					str[idx++] = s[j++];
				}
			} else {
				str[idx++] = '%';
				str[idx++] = 'l';
				str[idx++] = c;
			}
		} else {
			str[idx++] = '%';
			str[idx++] = c;
		}
	}

	return idx;
}

static char *int_to_string(char *buf, int n){
	int pos = NUM_BUF;
	int negative = 0;

	if(n < 0){
		negative = 1;
		n = -n;
	}

	buf[--pos] = 0;

	while(n >= 10) {
		int digit = n % 10;
		n /= 10;
		buf[--pos] = digit + '0';
	}
	buf[--pos] = n + '0';

	if(negative)
		buf[--pos] = '-';

	return &buf[pos];
}

static char *longlong_to_string(char *buf, unsigned long long n){
	int pos = NUM_BUF;

	buf[--pos] = 0;

	while(n >= 10) {
		int digit = n % 10;
		n /= 10;
		buf[--pos] = digit + '0';
	}

	buf[--pos] = n + '0';

	return &buf[pos];
}

static char *longlong_to_hexstring(char *buf, unsigned long long n){
	int pos = NUM_BUF;
	char table[] = {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
			'A', 'B', 'C', 'D', 'E', 'F'};

	buf[--pos] = 0;

	while(n >= 16) {
		int digit = n % 16;
		n /= 16;
		buf[--pos] = table[digit];
	}
	buf[--pos] = table[n];

	return &buf[pos];
}

#define TAB	8
void print_to_screen(char c) {
	int ncol, nrow;
	int t_sp;

	if (c != '\t' && c != '\n' && c !='\b' && c != '\r' && c != 0xe) {
		screen_buffer[cursor.row * MAX_COLUMN * 2 + cursor.col * 2] = c;
		screen_buffer[cursor.row * MAX_COLUMN * 2 + cursor.col * 2 + 1] = 0;

		if ((ncol = cursor.col + 1) != MAX_COLUMN)
			cursor.col = ncol;
		else {
			ncol = 0;
			nrow = cursor.row + 1;

			if (nrow == MAX_ROW) {
				scoll_screen_up();
				nrow = MAX_ROW - 1;
			}

			cursor.row = nrow;
			cursor.col = ncol;
		}
	} else if (c == '\n') {
		nrow = cursor.row + 1;
		ncol = 0;

		if (nrow == MAX_ROW) {
			scoll_screen_up();
			nrow = MAX_ROW - 1;
		}

		cursor.row = nrow;
		cursor.col = ncol;
	} else if (c == '\t') {
		t_sp = cursor.col % TAB;

		while(t_sp < TAB) {
			screen_buffer[cursor.row * MAX_COLUMN * 2 + cursor.col * 2] = ' ';
			screen_buffer[cursor.row * MAX_COLUMN * 2 + cursor.col * 2 + 1] = 0;
			cursor.col++;
			t_sp++;
		}

		ncol = cursor.col;

		if ((ncol = cursor.col + 1) != MAX_COLUMN)
			cursor.col = ncol;
		else {
			ncol = 0;
			nrow = cursor.row + 1;

			if (nrow == MAX_ROW) {
				scoll_screen_up();
				nrow = MAX_ROW - 1;
			}

			cursor.row = nrow;
			cursor.col = ncol;
		}
	} else if (c == 0xe) {	/* '\b' */
		if (stdin_buf_size > 0) {
			screen_buffer[cursor.row * MAX_COLUMN * 2 + (cursor.col - 1) * 2] = ' ';
			screen_buffer[cursor.row * MAX_COLUMN * 2 + (cursor.col - 1) * 2 + 1] = 0;
			cursor.col -= 1;
		}
	}

	return;
}

static void scoll_screen_up(void) {
	int i, j;
	for (i = 0; i < MAX_ROW - 1; i++)
		for (j = 0; j < MAX_COLUMN; j++) {
			screen_buffer[i * MAX_COLUMN * 2 + j * 2] = screen_buffer[(i + 1) * MAX_COLUMN * 2 + j * 2];
			screen_buffer[i * MAX_COLUMN * 2 + j * 2 + 1] = 0;
		}

	for (j = 0; j < MAX_COLUMN; j++) {
		screen_buffer[i * MAX_COLUMN * 2 + j * 2] = ' ';
		screen_buffer[i * MAX_COLUMN * 2 + j * 2 + 1] = 0;
	}

	return;
}

void clear_screen(void) {
	int i;
	for (i = 0; i < cursor.row; i++)
		scoll_screen_up();
	cursor.row = 0;
}
