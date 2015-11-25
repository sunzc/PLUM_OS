#ifndef _STRING_H
#define _STRING_H

#include <stdlib.h>

char *strcpy (char *__restrict, const char *__restrict);
char *strncpy (char *__restrict, const char *__restrict, size_t);

int strcmp (const char *, const char *);
int strncmp (const char *, const char *, size_t);

char *strchr (const char *, int);
char *strrchr (const char *, int);

size_t strlen (const char *);

#endif
