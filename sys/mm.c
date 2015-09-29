#include <sys/sbunix.h>
void *memset(void *s, int c, size_t n) {
	int i;
	char *p = (char *)s;

	for (i = 0; i < n; i++)
		*(p + i) = (char)c;

	return s;
}
