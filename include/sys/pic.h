#ifndef _PIC_H
#define _PIC_H

#include <sys/defs.h>

void picenable(int);
void picinit(void);
void pic_sendeoi(uint8_t irq);

#endif
