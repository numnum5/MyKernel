; ------------------------------
; General Protection Fault Handler (Vector 13)
; ------------------------------
%macro ISR_NO_ERR 1
isr_%1:
    push 0              ; fake error code
    push %1             ; interrupt number
    jmp isr_common
%endmacro

%macro ISR_ERR 1
isr_%1:
    push %1             ; interrupt number
    jmp isr_common
%endmacro

%macro GENERATE_ISRS 0
    ISR_NO_ERR 0
    ISR_NO_ERR 1
    ISR_NO_ERR 2
    ISR_NO_ERR 3
    ISR_NO_ERR 4
    ISR_NO_ERR 5
    ISR_NO_ERR 6
    ISR_NO_ERR 7

    ISR_ERR    8

    ISR_NO_ERR 9
    ISR_ERR    10
    ISR_ERR    11
    ISR_ERR    12
    ISR_ERR    13
    ISR_ERR    14

    ISR_NO_ERR 15
    ISR_NO_ERR 16
    ISR_ERR    17
    ISR_NO_ERR 18
    ISR_NO_ERR 19
    ISR_NO_ERR 20
    ISR_NO_ERR 21
    ISR_NO_ERR 22
    ISR_NO_ERR 23
    ISR_NO_ERR 24
    ISR_NO_ERR 25
    ISR_NO_ERR 26
    ISR_NO_ERR 27
    ISR_NO_ERR 28
    ISR_NO_ERR 29
    ISR_ERR    30
    ISR_NO_ERR 31
%endmacro

GENERATE_ISRS

section .data
global isr_table
isr_table:
%assign i 0
%rep 32
    dq isr_%+i
%assign i i+1
%endrep

extern gpf_handler_c
section .text
global gpf_handler

isr_common:
    ; --- Save general purpose registers ---
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

    ; --- Pass pointer to interrupt frame ---
    mov rdi, rsp
    call gpf_handler_c

    ; --- Restore registers ---
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

    ; --- Remove int_no + err_code ---
    add rsp, 16

    iretq


gpf_handler:
    cli                     ; Disable interrupts

; --- Save general purpose registers ---
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


    mov rdi, rsp
    call gpf_handler_c

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

    ; --- Infinite loop ---
