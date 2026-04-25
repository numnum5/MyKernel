simple x86-64 kernel written in C

Checklist:
- [x] Higher Half Kernel (at 0xffffffff80000000)
- [x] GDT (kernel + user segments)
- [x] IDT (interrupt table)
- [x] Physical Memory Manager (bitmap)
- [x] Virtual memory (paging setup)
- [ ] Syscall interface
- [ ] Process structure
- [ ] Context switching
- [ ] Simple scheduler (round-robin)
- [x] Kernel heap (`malloc`), borrowing design from FreeRTOS Heap4.c
- [x] Filesystem (FAT32)
- [x] ELF loader (in progress)
- [ ] User space

## Build
 - `make build-x86_64`

## Emulation
 - `qemu-system-x86_64 -cdrom dist/x86_64/kernel.iso`

## Shell:
<img width="1442" height="922" alt="image" src="https://github.com/user-attachments/assets/8ad82958-1be4-4a6b-b42a-9cb0109df35a" />
