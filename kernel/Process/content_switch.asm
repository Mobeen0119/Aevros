	[bits   32]

global switch_current_task
global read_eip


extern current_task
extern tss
extern dbg_hex32

CR3_OFFSET      equ     0
ESP_OFFSET      equ     28
EBP_OFFSET      equ     24
KERNEL_STACK_OFFSET equ     76
FIRST_RUN_OFFSET equ     232

switch_current_task:

	mov     eax, [esp+4]
	mov     ecx, [esp+8]

	test    eax, eax
	jz      .load_next

	pushf
	push    ebp
	push    ebx
	push    esi
	push    edi

	mov     [eax+ESP_OFFSET], esp
	mov     [eax+EBP_OFFSET], ebp

.load_next:

	mov     [current_task], ecx



	mov     edx, [ecx + KERNEL_STACK_OFFSET]
	mov     [tss + 4], edx

	mov     al, 'A'
	out     0xE9, al

	mov     esp, [ecx + ESP_OFFSET]

	mov     al, 'B'
	out     0xE9, al

	mov     ebp, [ecx + EBP_OFFSET]

	mov     al, 'C'
	out     0xE9, al

	cmp     dword [ecx + FIRST_RUN_OFFSET], 0

	mov     al, 'D'
	out     0xE9, al

	jne     .first_run

	mov     al, 'E'
	out     0xE9, al

.resume_normal:

	mov     al, 'F'
	out     0xE9, al

	pop     edi

	mov     al, 'G'
	out     0xE9, al

	pop     esi
	pop     ebx
	pop     ebp
	popf

	mov     esp, [ecx+ESP_OFFSET]
	mov     eax, [ecx+ESP_OFFSET]
	call    dbg_hex32

	mov     eax, [esi+12]
	call    dbg_hex32

	mov     eax, [esi+16]
	call    dbg_hex32

	mov     eax, [esi+20]
	call    dbg_hex32

	mov     eax, [esi+24]
	call    dbg_hex32

	mov     eax, [esi+28]
	call    dbg_hex32
	iret

.first_run:

	mov     dword [ecx + FIRST_RUN_OFFSET], 0

	cmp     dword [ecx + CR3_OFFSET], 0
	je      .kernel_segments


.user_segments:

	mov     ax, 0x23
	mov     ds, ax
	mov     es, ax
	mov     fs, ax
	mov     gs, ax
	jmp     .restore

.kernel_segments:

	mov     ax, 0x10
	mov     ds, ax
	mov     es, ax
	mov     fs, ax
	mov     gs, ax

.restore:

	pop     eax
	pop     ebp
	pop     ebx
	pop     esi
	pop     edi
	iret

read_eip:
	mov     eax, [esp]
	ret