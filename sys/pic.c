#include <sys/sbunix.h>
#include <sys/irq.h>

// borrow a lot from xv6 picirq.c
// A little findings about pic is that the EOI_AUTO bit is equivalent to
// adding pic_sendeoi() at the end of interrupt handler

#define IO_PIC1		0x20	// Master (IRQs 0-7)
#define IO_PIC2		0xA0	// Slave (IRQs 8-15)

#define IRQ_SLAVE	2	// IRQ at which slave connects to master
#define PIC_EOI		0x20		/* End-of-interrupt command code */

// Current IRQ mask
// Initial IRQ mask has interrupt 2 enabled (for slave 8259A)
static uint16_t irqmask = 0xFFFF & ~(1 << IRQ_SLAVE);

static void picsetmask(uint16_t mask) {
	irqmask = mask;
	outb(IO_PIC1 + 1, mask);
	outb(IO_PIC2 + 1, mask >> 8);
}

void picenable(int irq) {
	picsetmask(irqmask & ~(1<<irq));
}

// Initialize the 8259A interrupt controllers.
void picinit(void) {
	// mask all interrupts
	outb(IO_PIC1 + 1, 0xFF);
	outb(IO_PIC2 + 1, 0xFF);

	// Set up master (8259A-1)

	// ICW1:  0001g0hi
	//    g:  0 = edge triggering, 1 = level triggering
	//    h:  0 = cascaded PICs, 1 = master only
	//    i:  0 = no ICW4, 1 = ICW4 required
	outb(IO_PIC1, 0x11);

	// ICW2:  Vector offset
	outb(IO_PIC1 + 1, T_IRQ0);

	// ICW3:  (master PIC) bit mask of IR lines connected to slaves
	// 	  (slave PIC) 3-bit # of slave's connection to master
	outb(IO_PIC1 + 1, 1<<IRQ_SLAVE);

	// ICW4:  000nbmap
	//    n:  1 = special fully nestd mode
	//    b:  1 = buffered mode
	//    m:  0 = slave PIC, 1 = master PIC
	//	(ignored when b is 0, as the master/slave role
	//	can be hardwired).
	//    a: 1 = Automatic EOI mode
	//    p: 0 = MCS-80/85 mode, 1 = intel x86 mode
	outb(IO_PIC1 + 1, 0x1);

	// Set up slave (8259A-2)
	outb(IO_PIC2, 0x11);		// ICW1
	outb(IO_PIC2 + 1, T_IRQ0 + 8);	// ICW2
	outb(IO_PIC2 + 1, IRQ_SLAVE); 	// ICW3
	outb(IO_PIC2 + 1, 0x3);		// ICW4

	// 0CW3:  0ef01prs
	//   ef:  0x = NOP, 10 = clear specific mask, 11 = set specific mask
	//    p:  0 = no polling, 1 = polling mode
	//   rs:  0x = NOP, 10 = read IRR, 11 = read ISR
	outb(IO_PIC1, 0x68);		// clear specific mask
	outb(IO_PIC1, 0x0a);		// read IRR by default

	outb(IO_PIC2, 0x68);		// OCW3
	outb(IO_PIC2, 0x0a);		// OCW3

	if(irqmask != 0xFFFF)
		picsetmask(irqmask); 
}
 
void pic_sendeoi(uint8_t irq) {
	if(irq >= 8)
		outb(IO_PIC2,PIC_EOI);
 
	outb(IO_PIC1,PIC_EOI);
}
