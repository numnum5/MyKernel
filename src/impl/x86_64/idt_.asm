; extern idt_handler_keyboard
extern timer_interrupt_handler
extern switch_context


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



global scheduler_yield
scheduler_yield:
	; pushed by cpu: ss, rsp, rflags, cs, rip
	; save current thread
	push r15
	push r14
	push r13
	push r12
	push r11
	push r10
	push r9
	push r8
	push rdi
	push rsi
	push rdx
	push rcx
	push rbx
	push rax
	push rbp

	; swap thread context
	mov rdi, rsp
	call switch_context
	; load new thread
	pop rbp
	pop rax
	pop rbx
	pop rcx
	pop rdx
	pop rsi
	pop rdi
	pop r8
	pop r9
	pop r10
	pop r11
	pop r12
	pop r13
	pop r14
	pop r15
	
	; popped by cpu: rip, cs, rflags, rsp, ss
	iretq