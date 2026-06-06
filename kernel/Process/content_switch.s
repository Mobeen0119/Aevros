global switch_current_task
extern current_task
extern tss_entry

CR3_OFFSET      equ     0
KERNEL_STACK_OFFSET equ     88
ESP_OFFSET      equ     36
EBP_OFFSET      equ     40


switch_current_task:

	mov     eax, [esp+4]
	mov     ecx, [esp+8]

	test    eax, eax							; Check if current_task is NULL
	jz      .load_next							; If it is, skip saving state


	mov     [eax + ESP_OFFSET], esp
	mov     [eax + EBP_OFFSET], ebp

.load_next:

	mov     [current_task], ecx					; Update current_task to the new task

	mov     edx, [ecx+CR3_OFFSET]
	mov     cr3, edx

	mov     edx, [ecx + KERNEL_STACK_OFFSET]
	mov     [tss_entry + 4], edx				; Update the TSS's esp0 to the new task's kernel stack

	mov     esp, [ecx + ESP_OFFSET]
	mov     ebp, [ecx + EBP_OFFSET]

	iret


global read_eip

read_eip:
	mov     eax, [esp]
	jmp     eax
