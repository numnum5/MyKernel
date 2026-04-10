

#include "print.h"
#include "x86_64/string.h"
#include "x86_64/pmm.h"
#include "x86_64/idt.h"
extern uint32_t current_cluster;


int split_by_space(char *str, char **argv, int max_args)
{
    int argc = 0;

    while (*str && argc < max_args) {
        // skip leading spaces
        while (*str == ' ')
            str++;

        if (*str == '\0')
            break;

        // start of argument
        argv[argc++] = str;

        // find end of argument
        while (*str && *str != ' ')
            str++;

        if (*str == '\0')
            break;

        // terminate argument
        *str = '\0';
        str++;
    }

    return argc;
}

void shell(void)
{
    vga_clear();
    vga_print("Welcome to MyKernel\n");
    
    // search_entry()
    
    char * argv[8];
    char buf[256];
    
    while (1)
    {
        char * curr_dir = list_current_dir(current_cluster);
        vga_putc('~');
        vga_print(curr_dir);
        vga_print("$ ");
        read_line(buf, 256);

        uint8_t argc = split_by_space(buf, argv, 8);

        if (argc > 0) 
        {
            if (strcmp(argv[0], "ls") == 0) 
            {
                list_dir(current_cluster);
            }
            else if (strcmp(argv[0], "cd") == 0) 
            {
                char * dir_name = argv[1];
                vga_print(dir_name);     
                cd_dir(current_cluster, dir_name)    ;       
            }
        //         vga_print("FUCKU");
        // //         // shell_help();
        // //         vga_print("help\n");
        // //     } else if (strcmp2(argv[0], "echo") == 0) {
        // //         for (int i = 1; i < argc; i++)
        // //             vga_print(argv[i]);
        //     }
        }
        
        // }
    
        // vga_putc(c);
        // vga_print(buf);
        // vga_print(buf);
        //  printf("hello from kys\n");

        //  for(uint64_t i = 0; i < 1000000; i++);
        // asm volatile("hlt");
    };
}