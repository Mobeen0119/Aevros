	[global idt_load]

idt_load:
	mov,edx,[esp+4]
	lidt[edx]
	ret