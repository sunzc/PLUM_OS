#include <stdio.h>
#include <stdlib.h>

#define MAX_BUF_SIZE 4097 
#define NUM_BUF 80

static char *int_to_string(char *buf, int n);
static char *longlong_to_hexstring(char *buf, unsigned long long n);

int printf(const char *format, ...) {
	va_list val;
	char str[MAX_BUF_SIZE] = {0};
	int printed = 0;

	va_start(val, format);
	vsprintf(str, format, val);
	va_end(val);

	while(str[printed]) {
		write(1, str+printed, 1);
		++printed;
	}

	return printed;
}

int vsprintf(char *str, const char *fmt, va_list ap){
	char c;
	int n;
	unsigned long long ull;
	unsigned char uc;
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
		} else if(c == 'l'){
			c = *fmt++;
			if(c == 'x'){
				ull = va_arg(ap, unsigned long long);
				s = longlong_to_hexstring(buf, ull);
				j = 0;
				while(s[j] != 0){
					str[idx++] = s[j++];
				}
			} else {
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
