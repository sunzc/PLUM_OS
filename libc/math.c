#include <sbush.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int power(int radix, int n) {
	int sum = 1, i;

	for (i = 0; i < n; i++)
		sum *= radix;

	return sum;
}
	
int atoi(char *str) {
	int len, sum, i;

	sum = 0;
	len = strlen(str);
	for (i = 0; i < len; i++) {
		sum += power(10, len - i - 1) * (str[i] - '0');
		assert((str[i] - '0') >= 0 && (str[i] - '0') <= 9);
	}

	return sum;
}
