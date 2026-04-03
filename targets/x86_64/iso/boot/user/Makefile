# Toolchain
CC = x86_64-elf-gcc
LD = x86_64-elf-ld

# Flags
CFLAGS = -ffreestanding -nostdlib -O2 -Wall -Wextra -I. -MMD -MP
LDFLAGS = -T user.ld

# Files
SRC = user.c syscalls.c
OBJ = $(SRC:%.c=build/%.o)

OUT = user.elf

all: $(OUT)

build:
	mkdir -p build

# Compile
build/%.o: %.c | build
	$(CC) $(CFLAGS) -c $< -o $@

# Link
$(OUT): $(OBJ)
	$(LD) $(OBJ) -o $(OUT) $(LDFLAGS)

clean:
	rm -rf build $(OUT)

-include $(OBJ:.o=.d)