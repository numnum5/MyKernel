
; global syscall_entry
extern switch_context
extern switch_context2
extern fucklife
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

; global start_first_process

; start_first_process:
;     mov rsp, [first_proc_tf]
;     iretq
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


global start_first_thread

start_first_thread:
    mov rsp, rdi            ; switch to top of user stack
    iretq


global jump_usermode
extern test_user_function
jump_usermode:
    ; 1. Clear out the segment registers
    ; In 64-bit mode, DS, ES, FS, GS are mostly ignored but 
    ; should point to a valid User Data selector (DPL 3).
    mov ax, (4 * 8) | 3      ; User Data Selector (Index 4, RPL 3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax 

    ; 2. Prepare the iretq stack frame
    ; The stack MUST be 16-byte aligned before this if you plan 
    ; on calling functions in Ring 3 that use SSE/AVX.
    
    mov rax, rsp             ; Save current RSP to pass it back as User RSP
    
    ; Push the 5-part frame (Order: SS, RSP, RFLAGS, CS, RIP)
    push qword (4 * 8) | 3   ; SS (User Data Selector)
    push rax                 ; RSP (User Stack Pointer)
    
    pushfq                   ; RFLAGS
    ; Optional: If you want to force interrupts ON in user mode:
    ; pop rax
    ; or rax, 0x200          ; Set IF bit
    ; push rax

    push qword (3 * 8) | 3   ; CS (User Code Selector, Index 3, RPL 3)
    push test_user_function  ; RIP (Target address)

    ; 3. The Great Leap
    ; If you use GS for kernel data, execute SWAPGS here.
    ; swapgs 
    
    iretq