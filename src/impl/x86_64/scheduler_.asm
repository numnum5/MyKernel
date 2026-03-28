
; global syscall_entry
extern switch_context
extern switch_context2
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
	; mov [rdi], rsp

	call switch_context
	
    ; mov rsp, rax
	; mov rsp, rax
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


extern syscall_handler

; syscall_entry:
;     mov [user_rsp], rsp
;     mov rsp, kernel_stack_top

;     ; build iret frame
;     push 0x23
;     push qword [user_rsp]
;     push r11
;     push 0x1B
;     push rcx

;     ; save callee-saved regs ONLY
;     push rax
;     push rbx
;     push rbp
;     push rdi
;     push rsi
;     push rdx
;     push r10
;     push r8
;     push r9
;     push r12
;     push r13
;     push r14
;     push r15

;     call syscall_handler

;     ; restore
;     pop r15
;     pop r14
;     pop r13
;     pop r12
;     pop r9
;     pop r8
;     pop r10
;     pop rdx
;     pop rsi
;     pop rdi
;     pop rbp
;     pop rbx
;     pop rax

;     iretq

