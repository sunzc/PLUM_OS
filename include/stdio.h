#ifndef _STDIO_H
#define _STDIO_H

#include <stdarg.h>

int printf(const char *format, ...);
int vsprintf(char *str, const char *fmt, va_list ap);

#endif
