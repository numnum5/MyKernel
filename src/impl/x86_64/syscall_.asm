extern syscall_handler
global syscall_entry
%macro pushaq 0
push rax
push rbx
push rcx
push rdx
push rbp
push rdi
push rsi
push r8
push r9
push r10
push r11
push r12
push r13
push r14
push r15
%endmacro

%macro popaq 0
pop r15
pop r14
pop r13
pop r12
pop r11
pop r10
pop r9
pop r8
pop rsi
pop rdi
pop rbp
pop rdx
pop rcx
pop rbx
pop rax
%endmacro


; This is called form user space, meaning you cannot call it in kernel code (there's no need to anyway)
syscall_entry:
    cli
    ; swapgs
    ; Store the user stack and restore the kernel stack
    mov [gs:16], rsp
    mov rsp, [gs:8]
    pushaq
    ; cld
    mov rdi, rsp
    call syscall_handler
    ; popaq

    ; ; Return from the syscall
    ; push 0x23 ; SS
    ; push qword [gs:16] ; RSP
    ; push r11 ; RFLAGS
    ; push 0x1B ; CS
    ; push rcx ; RIP

    ; iretq