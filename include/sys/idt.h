#ifndef _IDT_H
#define _IDT_H

#include <sys/defs.h>

struct idt_entry_struct {
	uint16_t base_lo_16;
	uint16_t sel;
	uint8_t  always0;
	uint8_t  flags;
	uint16_t base_hi_16;
	uint32_t base_up_32;
	uint32_t reserved;
} __attribute__((packed));
typedef struct idt_entry_struct idt_entry_t;

struct idt_ptr_struct {
	uint16_t limit;
	uint64_t base;
} __attribute__((packed));
typedef struct idt_ptr_struct idt_ptr_t;

void init_idt(void);

#endif
