#ifndef _DIRENT_H
#define _DIRENT_H

#include <sys/defs.h>

#define NAME_MAX 255
struct dirent
{
  long d_ino;
  off_t d_off;
  unsigned short d_reclen;
	unsigned char d_type;
  char d_name [NAME_MAX+1];
};

#endif
