#include <stdlib.h>
#include <string.h>
int gethostname(char *buf, size_t size){
	strncpy(buf,"sbhost",size);
	return 0;
}

