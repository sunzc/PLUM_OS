#include <sys/sbunix.h>
#include <sys/idt.h>
#include <sys/mm.h>

/* record the time since boot */
uint64_t ticks = 0;

extern void _idt_load(void);
extern void _isr_wrapper_timer(void);
extern void _isr_wrapper_kbd(void);

static void idt_set_gate(uint8_t, uint64_t, uint16_t, uint8_t);

idt_entry_t idt_entries[256];
idt_ptr_t _idtp;

void init_idt() {
	_idtp.limit = sizeof(idt_entry_t) * 256 - 1;
	_idtp.base = (uint64_t)&idt_entries;

	memset(&idt_entries, 0, sizeof(idt_entry_t) * 256);
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
