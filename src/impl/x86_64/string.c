#include "x86_64/string.h"

int strcmp(const char *a, const char *b)
{
    while (*a && (*a == *b)) {
        a++;
        b++;
    }

    return (unsigned char)*a - (unsigned char)*b;
}


void strcpy(char * dest, char * source)
{
    while(*source)
    {
        *dest = *source;
        dest++;
        source++;
    }
    *dest = '\0';
}