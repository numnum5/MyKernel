; extern idt_handler_keyboard
extern timer_interrupt_handler
extern switch_context
extern default_handler

global idt_load

idt_load:
	lidt [rdi]
	ret

%macro WRAPPED_HANDLER 1
	global %1_wrapped
	
	%1_wrapped:
		; save general-purpose registers
		push rax
		push rbx
		push rcx
		push rdx
		push rbp
		push rsi
		push rdi
		push r8
		push r9
		push r10
		push r11
		push r12
		push r13
		push r14
		push r15

		call %1

		; restore general-purpose registers
		pop r15
		pop r14
		pop r13
		pop r12
		pop r11
		pop r10
		pop r9
		pop r8
		pop rdi
		pop rsi
		pop rbp
		pop rdx
		pop rcx
		pop rbx
		pop rax
		
		iretq
%endmacro

; WRAPPED_HANDLER idt_handler_keyboard
WRAPPED_HANDLER timer_interrupt_handler
WRAPPED_HANDLER default_handler


global enter_user_mode

enter_user_mode:
    ; rdi = user rip
    ; rsi = user rsp

    cli

    mov ax, 0x23        ; user data selector
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push qword 0x23     ; SS
    push rsi            ; RSP (user stack)
    pushfq              ; RFLAGS
    pop rax
    or rax, 0x200       ; enable interrupts in user mode
    push rax
    push qword 0x2B     ; CS (user code)
    push rdi            ; RIP (user entry point)

    iretq