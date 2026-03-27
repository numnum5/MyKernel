#include "print.h"
#include <stdarg.h>
#define VIRT_BASE 0xffffffff80000000
const static size_t NUM_COLS = 80;
const static size_t NUM_ROWS = 25;

struct Char {
    uint8_t character;
    uint8_t color;
};

struct Char* buffer = (struct Char*)(0xb8000 + VIRT_BASE);
size_t col = 0;
size_t row = 0;
uint8_t color = PRINT_COLOR_WHITE | PRINT_COLOR_BLACK << 4;

void clear_row(size_t row) {
    struct Char empty = (struct Char) {
        character: ' ',
        color: color,
    };

    for (size_t col = 0; col < NUM_COLS; col++) {
        buffer[col + NUM_COLS * row] = empty;
    }
}

void print_clear() {
    for (size_t i = 0; i < NUM_ROWS; i++) {
        clear_row(i);
    }
}

void print_newline() {
    col = 0;

    if (row < NUM_ROWS - 1) {
        row++;
        return;
    }

    for (size_t row = 1; row < NUM_ROWS; row++) {
        for (size_t col = 0; col < NUM_COLS; col++) {
            struct Char character = buffer[col + NUM_COLS * row];
            buffer[col + NUM_COLS * (row - 1)] = character;
        }
    }

    clear_row(NUM_ROWS - 1);
}

void print_char(char character) {
    if (character == '\n') {
        print_newline();
        return;
    }

    if (col > NUM_COLS) {
        print_newline();
    }

    buffer[col + NUM_COLS * row] = (struct Char) {
        character: (uint8_t) character,
        color: color,
    };

    col++;
}

void print_str(char* str) {
    for (size_t i = 0; 1; i++) {
        char character = (uint8_t) str[i];

        if (character == '\0') {
            return;
        }

        print_char(character);
    }
}

void print_set_color(uint8_t foreground, uint8_t background) {
    color = foreground + (background << 4);
}

void print_uint64_dec(uint64_t value) {
    if (value == 0) {
        print_char('0');
        return;
    }
    
    char buffer[20];
    int i = 0;
    
    while (value > 0) {
        buffer[i++] = (value % 10) + '0';
        value /= 10;
    }
    
    while (i-- > 0) {
        print_char(buffer[i]);
    }
}




void printf(const char* format, ...) {
    va_list args;
    va_start(args, format);

    for (int i = 0; format[i] != '\0'; i++) {
        if (format[i] == '%') {
            i++;
            switch (format[i]) {
                case 's': {
                    char* s = va_arg(args, char*);
                    while (*s) print_char(*s++);
                    break;
                }
                case 'd': {
                    int d = va_arg(args, int);
                    print_uint64_dec(d); // You'll need a helper to convert int to string
                    break;
                }
                case 'x': {
                    uint64_t x = va_arg(args, uint64_t);
                    print_uint64_hex(x); // Crucial for debugging addresses!
                    break;
                }
                default:
                    print_char('%');
                    print_char(format[i]);
                    break;
            }
        } else {
            print_char(format[i]);
        }
    }

    va_end(args);
}

void print_int(int64_t n) {
    if (n == 0) {
        print_char('0');
        return;
    }

    if (n < 0) {
        print_char('-');
        // Handle INT64_MIN edge case where -n overflows
        if (n == -9223372036854775808LL) {
            print_str("9223372036854775808");
            return;
        }
        n = -n;
    }

    char buffer[32]; // Enough for a 64-bit integer
    int i = 0;

    // Extract digits into the buffer
    while (n > 0) {
        buffer[i++] = (n % 10) + '0'; // Convert digit to ASCII
        n /= 10;
    }

    // Print the buffer in reverse
    while (i > 0) {
        print_char(buffer[--i]);
    }
}

void print_uint64_hex(uint64_t value) {
    if (value == 0) {
        print_char('0');
        return;
    }
    
    char buffer[16];
    int i = 0;
    
    while (value > 0) {
        uint8_t digit = value & 0xF;
        
        if (digit < 10) {
            buffer[i++] = digit + '0';
        } else {
            buffer[i++] = digit - 10 + 'A';
        }
        
        value >>= 4;
    }
    
    while (i-- > 0) {
        print_char(buffer[i]);
    }
}

void print_uint64_bin(uint64_t value) {
    char buffer[64];
    
    for (size_t i = 0; i < 64; i++) {
        buffer[i] = (value & 1) + '0';
        value >>= 1;
    }
    
    for (size_t i = 64; i > 0; i--) {
        print_char(buffer[i - 1]);
    }
}
