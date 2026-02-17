OUTPUT_FORMAT(elf64-x86-64)
ENTRY(start)

KERNEL_PHYS = 0x200000;
KERNEL_VIRT = 0xffffffff80000000;

SECTIONS
{
	. = 1M;

	.boot :
    {
        *(.multiboot)
    }

	. += KERNEL_VIRT;

	.text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VIRT) 
	{
		*(.text*)
	}

	.rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VIRT)
	{
		*(.rodata*)
	}

	.data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VIRT)
	{
		
		*(.data*)
	}

	.bss ALIGN(4K) : AT(ADDR(.bss) - KERNEL_VIRT)
	{
		*(.bss*)
		*(.page_tables*)
		*(.stack*)
		. = ALIGN(4K);
		__kernel_heap_start = .;
		*(.heap)
		__kernel_heap_end = .;
	}

	/DISCARD/ :
	{
		*(.note*)
		*(.comment*)
	}
}