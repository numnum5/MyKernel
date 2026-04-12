

#include "print.h"
#include "x86_64/string.h"
#include "x86_64/pmm.h"
#include "x86_64/idt.h"
extern uint32_t current_cluster;


int split_by_space(char *str, char **argv, int max_args)
{
    int argc = 0;

    while (*str && argc < max_args) {
        while (*str == ' ')
            str++;

        if (*str == '\0')
            break;
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

void print_banner() {
     print_set_color(PRINT_COLOR_MAGENTA, PRINT_COLOR_BLACK);
    printf("##################################################\n");
    printf("#   __  ____   __  __ __ ____ __  _  _ ____ _    #\n");
    printf("#  |  \\/  \\ \\ / /  | |/ /| ___|\\ \\| \\| | ___| |  #\n");
    printf("#  | |\\/| |\\ V /   |   < | |__  | .   | |_  | |  #\n");
    printf("#  |_|  |_| |_|    |_|\\_\\|____|_|\\_|\\__|___||_|  #\n");
    printf("#                                                #\n");
    printf("##################################################\n");
}

void help(void) {
    print_set_color(PRINT_COLOR_BLUE, PRINT_COLOR_BLACK);
    printf("+----------------------------------------+\n");
    printf("|           MY KERNEL - HELP             |\n");
    printf("+----------------------------------------+\n");
    printf("|  COMMAND       DESCRIPTION             |\n");
    printf("+----------------------------------------+\n");
    printf("|  help          show this menu          |\n");
    printf("|  echo          print text              |\n");
    printf("|  clear         clear screen            |\n");
    printf("|  pwd           print directory         |\n");
    printf("|  ls            list files              |\n");
    printf("|  cd            change directory        |\n");
    printf("|  mkdir         make directory          |\n");
    printf("|  rm            remove file             |\n");
    printf("|  cat           print file              |\n");
    printf("|  ps            list processes          |\n");
    printf("|  kill          kill process            |\n");
    printf("|  exit          quit shell              |\n");
    printf("+----------------------------------------+\n");
}

void shell(void)
{
    vga_enable_cursor();
    print_clear();
    print_banner();
    
    // search_entry()
    char * argv[8];
    char buf[256];

    char * dir_names[20];
    while (1)
    {
        uint8_t index = pwd(current_cluster, dir_names);
        print_set_color(PRINT_COLOR_WHITE, PRINT_COLOR_BLACK);
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
            else if (strcmp(argv[0], "echo") == 0)
            {
                printf("%s\n", argv[1]);
            }
            else if (strcmp(argv[0], "clear") == 0)
            {
                print_clear();
            }
            else if (strcmp(argv[0], "help") == 0)
            {
                help();
            }
        }
    };
}