CC      := clang
LD      := ld.lld
NASM 	:= nasm

CFLAGS  := \
    -target x86_64-unknown-none-elf \
    -ffreestanding \
    -nostdlib \
    -mno-sse \
    -mno-sse2 \
    -mno-mmx \
    -mno-80387 \
    -Wall \
    -Wextra \
    -O2 \
    -Iinclude

LDFLAGS := \
    -T linker.ld

OBJS = \
    crt0.o \
    libc/syscall.o \
    init/init.o

all: init.elf

crt0.o:
	$(NASM) -f elf64 crt0.S -o crt0.o

libc/syscall.o:
	$(CC) $(CFLAGS) -c libc/syscall.c -o libc/syscall.o

init/init.o:
	$(CC) $(CFLAGS) -Iinclude -c init/init.c -o init/init.o

init.elf: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o init.elf

clean:
	rm -f *.o
	rm -f libc/*.o
	rm -f init/*.o
	rm -f init.elf