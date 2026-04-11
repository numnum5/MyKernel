

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
    vga_enable_cursor();
    print_clear();
    printf("Welcome to MyKernel\n");
    
    // search_entry()
    char * argv[8];
    char buf[256];
    while (1)
    {
        char * curr_dir = list_current_dir(current_cluster);
        print_char('~');
        printf(curr_dir);
        printf("$ ");
        tty_readline(buf, 256);
        print_char('\n');
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
                // vga_print(dir_name);     
                cd_dir(current_cluster, dir_name)    ;       
            }
        }
    };
}