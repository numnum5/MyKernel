

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


void pwd_print(uint8_t index, char ** names)
{
    for(int8_t i = index - 1; i >= 0; i--)
    {
        if (i != 0)
        {
            printf("/%s", names[i]);
        }
        else
        {
            printf("/%s\n", names[i]);   
        }
    }
}

void print_dirs(uint8_t index, char ** names)
{

    // printf("index: %d\n", index);
    if (index == 0)
    {
        printf("%s", names[0]);
        return;
    }

    for(int8_t i = index - 1; i >= 0; i--)
    {
        printf("/%s", names[i]);
  
    }
}
void shell(void)
{
    vga_enable_cursor();
    print_clear();
    printf("Welcome to MyKernel\n");
    
    // search_entry()
    char * argv[8];
    char buf[256];

    char * dir_names[20];
    while (1)
    {
        uint8_t index = pwd(current_cluster, dir_names);
        print_char('~');        
        print_dirs(index, dir_names);
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
                cd_dir(current_cluster, dir_name)    ;       
            }
            else if (strcmp(argv[0], "mkdir") == 0)
            {
                char * dir_name = argv[1];

                fat32_mkdir(current_cluster, dir_name);
            }
            else if (strcmp(argv[0], "pwd") == 0)
            {
                // char * dir_name = argv[1];
                // uint8_t index = 0;
                uint8_t index = pwd(current_cluster, dir_names);
                pwd_print(index, dir_names);
            }
        }
    };
}