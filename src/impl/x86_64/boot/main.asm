%define VIRT_BASE 0xffffffff80000000

global start
global page_table_l4
global stack_bottom
global stack_top
global page_table_l3
global page_table_l2
global stack_tss_bottom
global stack_tss_top
global gdt64
extern long_mode_start

section .text
bits 32
start:
	cli
	mov esp, stack_top - VIRT_BASE

		; Move magic number and pointer to arguement registers
	mov esi, ebx
	mov edi, eax

	call check_multiboot
	call check_cpuid
	call check_long_mode
	call init_pages
	call enable_paging

	lgdt [gdt64.pointer - VIRT_BASE]
	jmp gdt64.kcode:(long_mode_start - VIRT_BASE)

	hlt

check_multiboot:
	cmp eax, 0x36d76289
	jne .no_multiboot
	ret
.no_multiboot:
	mov al, "M"
	jmp error

check_cpuid:
	pushfd
	pop eax
	mov ecx, eax
	xor eax, 1 << 21
	push eax
	popfd
	pushfd
	pop eax
	push ecx
	popfd
	cmp eax, ecx
	je .no_cpuid
	ret
.no_cpuid:
	mov al, "C"
	jmp error

check_long_mode:
	mov eax, 0x80000000
	cpuid
	cmp eax, 0x80000001
	jb .no_long_mode

	mov eax, 0x80000001
	cpuid
	test edx, 1 << 29
	jz .no_long_mode
	
	ret

.no_long_mode:
	mov al, "L"
	jmp error

init_pages:
	mov eax, page_table_l4 - VIRT_BASE
	or eax, 0b11	; writeable, present
	mov [page_table_l4 - VIRT_BASE + 8 * 510], eax	; Recursive map

	mov eax, page_table_l3 - VIRT_BASE
	or eax, 0b11	; writeable, present
	mov [page_table_l4 - VIRT_BASE], eax			; Lower-half identity map
	mov [page_table_l4 - VIRT_BASE + 8 * 511], eax	; Higher-half identity map

	mov eax, page_table_l2 - VIRT_BASE
	or eax, 0b11	; writeable, present
	mov [page_table_l3 - VIRT_BASE], eax			; Lower-half identity map
	mov [page_table_l3 - VIRT_BASE + 8 * 510], eax	; Higher-half identity map




	; mov eax, page_table_l1 - VIRT_BASE
	; or eax, 0b11	; writeable, present
	; mov [page_table_l2 - VIRT_BASE], eax			; Lower-half identity map
	; mov [page_table_l2 - VIRT_BASE + 8 * 510], eax	; Higher-half identity map




	mov ecx, 0 ; counter
.loop:

	mov eax, 0x200000 ; 2MiB
	mul ecx
	or eax, 0b10000011 ; present, writable, huge page
	mov [page_table_l2 - VIRT_BASE + ecx * 8], eax

	inc ecx ; increment counter
	cmp ecx, 512 ; checks if the whole table is mapped
	jne .loop ; if not, continue

	ret

enable_paging:
	; pass page table location to cpu
	mov eax, page_table_l4 - VIRT_BASE
	mov cr3, eax

	; enable PAE
	mov eax, cr4
	or eax, 1 << 5
	mov cr4, eax

	; enable long mode
	mov ecx, 0xC0000080
	rdmsr
	or eax, 1 << 8
	wrmsr

	; enable paging
	mov eax, cr0
	or eax, 1 << 31
	mov cr0, eax

	ret

error:
	; print "ERR: X" where X is the error code
	mov dword [0xb8000], 0x4f524f45
	mov dword [0xb8004], 0x4f3a4f52
	mov dword [0xb8008], 0x4f204f20
	mov byte  [0xb800a], al
	hlt

section .page_tables
align 4096

page_table_l4:
	resb 4096
page_table_l3:
	resb 4096
page_table_l2:
	resb 4096
page_table_l1:
	resb 4096
stack_bottom:
	resb 4096 * 10
stack_top:
stack_tss_bottom:
	resb 4096
stack_tss_top:

section .rodata

global ucode_selector 

global udata_selector       ; Make it visible to the linker
ucode_selector: equ gdt64.ucode ; Create a C-friendly alias
udata_selector: equ gdt64.udata ; Create a C-friendly alias
gdt64:
	dq 0												; Zero entry
.kcode: equ $ - gdt64
	dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)	; Kernel code segment entry
	; executable, code/data type, present, 64-bit
.kdata: equ $ - gdt64
	dq (1 << 41) | (1 << 44) | (1 << 47) | (1 << 53)	; Kernel data segment entry
	; writeable, code/data type, present, 64-bit
.ucode: equ $ - gdt64
	dq (1 << 43) | (1 << 41) | (1 << 44) | (3 << 45) | (1 << 47) | (1 << 53)	; User code segment entry
	; executable, code/data type, user mode, present, 64-bit
.udata: equ $ - gdt64
	dq (1 << 41) | (1 << 44) | (3 << 45) | (1 << 47) | (1 << 53)	; User data segment entry
	; writeable, code/data type, user mode, present, 64-bit
.tss: equ $ - gdt64
	resb 16	; Task state segment entry (filled programmatically later in boot process)
.pointer:					; Value used by LGDT
	dw $ - gdt64 - 1		; Length of GDT
	dq gdt64 - VIRT_BASE	; Address of GDT

; global long_mode_start
extern kernel_main

section .text
bits 64
long_mode_start:
    ; load null into all data segment registers
    ; mov ax, 0
    ; mov ss, ax
    ; mov ds, ax
    ; mov es, ax
    ; mov fs, ax
    ; mov gs, ax

	; call kernel_main
    ; hlt 

	mov ax, gdt64.kdata
	mov ds, ax
	mov es, ax
	mov fs, ax
	mov gs, ax
	mov ss, ax

	; Update GDT pointer to use higher-half address
	lea rax, gdt64
	mov [gdt64.pointer + 2], rax

	; Reload GDT in long mode
	lgdt [gdt64.pointer]
	lea rax, high_mem_entry
	jmp rax

	hlt

section .text
high_mem_entry:
	; Reset stack pointers
	mov rsp, stack_top
	xor rbp, rbp

	; mov rax, rsp
	; call print_hex64
	; COULD you make a simple code to show page table l4 is acccessbile via virt address
; [page_table_l4],

	; ; Unmap lower-half identity mapping
	mov rax, 0
	mov [page_table_l4], rax
	mov rax, cr3
	mov cr3, rax


	; Finally, go to main kernel function
	call kernel_main

	hlt



; -----------------------------------------------------------------------------
; print_hex64:
;   Input: RAX = 64-bit number to print
;   Prints RAX as 16-character hex to VGA text buffer (0xB8000)
;   Assumes white on black color (0x0F)
; -----------------------------------------------------------------------------
section .text
bits 64

print_hex64:
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi

    mov rsi, 0xB8000       ; VGA buffer
    mov rcx, 16            ; 16 hex digits

print_loop:
    mov rbx, rax           ; copy number
    shr rbx, 60            ; get highest nibble
    and bl, 0xF            ; isolate 4 bits

    ; convert to ASCII
    cmp bl, 10
    jl .digit
    add bl, 'A' - 10
    jmp .store
.digit:
    add bl, '0'

.store:
    mov [rsi], bl
    mov byte [rsi+1], 0x0F     ; color attribute
    add rsi, 2

    shl rax, 4                 ; shift left by 4 bits (next nibble)
    loop print_loop

    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    ret