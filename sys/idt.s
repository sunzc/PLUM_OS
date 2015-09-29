#
# idt.s
#

.text

#
# load a new IDT
#	parameter 1: address of idt_entries
.global _idt_load
.extern _idtp
_idt_load:
	lidt _idtp
	retq
