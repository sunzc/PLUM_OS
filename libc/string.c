#include <stdlib.h>
#include <string.h>

char *strcpy (char *dest, const char *src){
	int i = 0;

	if(dest == NULL || src == NULL)
		return NULL;

	while(i < strlen(src) && (dest[i] = src[i]) != '\0')
		i++;

	dest[i - 1] = '\0';
	return dest;
}
char *strncpy (char *dest, const char *src, size_t size){
	int i = 0;

	if(dest == NULL || src == NULL)
		return NULL;

	while(i < size && i < strlen(src) &&  (dest[i] = src[i]) != '\0')
		i++;

	dest[i] = '\0';
	return dest;
}

int strcmp (const char *dest, const char *src){
	int i = 0;

	if(dest == NULL || src == NULL)
		return -1;

	if(strlen(dest) != strlen(src))
		return -1;

	while((dest[i] - src[i]) == 0 && i < strlen(dest))
		i++;

	if(i == strlen(dest))
		return 0;
	else
		return dest[i] - src[i];
} 

int strncmp (const char *dest, const char *src, size_t size){
	int i = 0;

	if(dest == NULL || src == NULL)
		return -1;

	while((dest[i] - src[i]) == 0 && i < strlen(dest) && i < strlen(src) && i < size)
		i++;

	if(i == size)
		return 0;
	else
		return dest[i] - src[i];
} 

char *strchr (const char *s, int c){
	if(s == NULL)
		return NULL;

	while( *s != (char)c){
		if(!*s)
			return NULL;
		s++;
	}

	return (char *)s;
}

size_t strlen (const char *s){
	size_t size = 0;

	if(s == NULL)
		return 0;

	while(s[size] != '\0')
		size++;
	return size;
} 

