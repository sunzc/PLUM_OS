#include <sys/sbunix.h>
#include <sys/idt.h>
#include <sys/mm.h>

#define exception_handler_func(n)	\
	void exception_handler_##n (void) { printf("exception :%d\n",n); panic("exception!");}

#define exception_handler(n)	\
	exception_handler_##n

exception_handler_func(0)
exception_handler_func(1)
exception_handler_func(2)
exception_handler_func(3)
exception_handler_func(4)
exception_handler_func(5)
exception_handler_func(6)
exception_handler_func(7)
exception_handler_func(8)
exception_handler_func(9)
exception_handler_func(10)
exception_handler_func(11)
exception_handler_func(12)
exception_handler_func(13)
exception_handler_func(14)
exception_handler_func(15)
exception_handler_func(16)
exception_handler_func(17)
exception_handler_func(18)
exception_handler_func(19)
exception_handler_func(20)
exception_handler_func(21)
exception_handler_func(22)
exception_handler_func(23)
exception_handler_func(24)
exception_handler_func(25)
exception_handler_func(26)
exception_handler_func(27)
exception_handler_func(28)
exception_handler_func(29)
exception_handler_func(30)
exception_handler_func(31)
/* record the time since boot */
uint64_t ticks = 0;

extern void _idt_load(void);
extern void _isr_wrapper_timer(void);
extern void _isr_wrapper_kbd(void);
extern void _page_fault_handler(void);

static void idt_set_gate(uint8_t, uint64_t, uint16_t, uint8_t);

idt_entry_t idt_entries[256];
idt_ptr_t _idtp;

void init_idt() {
	_idtp.limit = sizeof(idt_entry_t) * 256 - 1;
	_idtp.base = (uint64_t)&idt_entries;

	memset(&idt_entries, 0, sizeof(idt_entry_t) * 256);
	idt_set_gate(0, (uint64_t)exception_handler(0), 0x08, 0x8f);
	idt_set_gate(1, (uint64_t)exception_handler(1), 0x08, 0x8f);
	idt_set_gate(2, (uint64_t)exception_handler(2), 0x08, 0x8f);
	idt_set_gate(3, (uint64_t)exception_handler(3), 0x08, 0x8f);
	idt_set_gate(4, (uint64_t)exception_handler(4), 0x08, 0x8f);
	idt_set_gate(5, (uint64_t)exception_handler(5), 0x08, 0x8f);
	idt_set_gate(6, (uint64_t)exception_handler(6), 0x08, 0x8f);
	idt_set_gate(7, (uint64_t)exception_handler(7), 0x08, 0x8f);
	idt_set_gate(8, (uint64_t)exception_handler(8), 0x08, 0x8f);
	idt_set_gate(9, (uint64_t)exception_handler(9), 0x08, 0x8f);
	idt_set_gate(10, (uint64_t)exception_handler(10), 0x08, 0x8f);
	idt_set_gate(11, (uint64_t)exception_handler(11), 0x08, 0x8f);
	idt_set_gate(12, (uint64_t)exception_handler(12), 0x08, 0x8f);
//	idt_set_gate(13, (uint64_t)exception_handler(13), 0x08, 0x8f);
	idt_set_gate(13, (uint64_t)_page_fault_handler, 0x08, 0x8f);
	idt_set_gate(14, (uint64_t)_page_fault_handler, 0x08, 0x8f);
//	idt_set_gate(14, (uint64_t)exception_handler(14), 0x08, 0x8f);
	idt_set_gate(15, (uint64_t)exception_handler(15), 0x08, 0x8f);
	idt_set_gate(16, (uint64_t)exception_handler(16), 0x08, 0x8f);
	idt_set_gate(17, (uint64_t)exception_handler(17), 0x08, 0x8f);
	idt_set_gate(18, (uint64_t)exception_handler(18), 0x08, 0x8f);
	idt_set_gate(19, (uint64_t)exception_handler(19), 0x08, 0x8f);
	idt_set_gate(20, (uint64_t)exception_handler(20), 0x08, 0x8f);
	idt_set_gate(21, (uint64_t)exception_handler(21), 0x08, 0x8f);
	idt_set_gate(22, (uint64_t)exception_handler(22), 0x08, 0x8f);
	idt_set_gate(23, (uint64_t)exception_handler(23), 0x08, 0x8f);
	idt_set_gate(24, (uint64_t)exception_handler(24), 0x08, 0x8f);
	idt_set_gate(25, (uint64_t)exception_handler(25), 0x08, 0x8f);
	idt_set_gate(26, (uint64_t)exception_handler(26), 0x08, 0x8f);
	idt_set_gate(27, (uint64_t)exception_handler(27), 0x08, 0x8f);
	idt_set_gate(28, (uint64_t)exception_handler(28), 0x08, 0x8f);
	idt_set_gate(29, (uint64_t)exception_handler(29), 0x08, 0x8f);
	idt_set_gate(30, (uint64_t)exception_handler(30), 0x08, 0x8f);
	idt_set_gate(31, (uint64_t)exception_handler(31), 0x08, 0x8f);
	idt_set_gate(32, (uint64_t)_isr_wrapper_timer, 0x08, 0x8E);
	idt_set_gate(33, (uint64_t)_isr_wrapper_kbd, 0x08, 0x8E);

	_idt_load();
}

static void idt_set_gate(uint8_t num, uint64_t base, uint16_t sel, uint8_t flags) {
	idt_entries[num].base_lo_16 = base & 0xFFFF;
	idt_entries[num].base_hi_16 = (base>>16) & 0xFFFF;
	idt_entries[num].base_up_32 = (base>>32) & 0xFFFFFFFF;

	idt_entries[num].sel = sel;
	idt_entries[num].always0 = 0;

	idt_entries[num].flags = flags;	
}
