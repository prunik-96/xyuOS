CC = i686-linux-gnu-gcc
LD = i686-linux-gnu-ld
AS = nasm

CFLAGS  = -ffreestanding -O2 -Wall -Wextra -m32 -fno-pic -fno-pie -fno-stack-protector
LDFLAGS = -T linker.ld -nostdlib -m elf_i386

ISO_DIR = iso
ISO_OUT = myos.iso

OBJS = \
  src/apps/editor.o \
  src/boot/boot.o \
  src/kernel/kernel.o \
  src/drivers/vga.o \
  src/drivers/pic.o \
  src/drivers/pit.o \
  src/drivers/keyboard.o \
  src/cpu/idt.o \
  src/cpu/irq.o \
  src/cpu/isr.o \
  src/cpu/gdt.o \
  src/cpu/gdt_flush.o \
  src/util/string.o

all: $(ISO_OUT)

# --- ASM ---
src/boot/boot.o: src/boot/boot.s
	$(AS) -f elf32 $< -o $@

src/cpu/isr.o: src/cpu/isr.s
	$(AS) -f elf32 $< -o $@

src/cpu/gdt_flush.o: src/cpu/gdt_flush.s
	$(AS) -f elf32 $< -o $@

# --- C ---
src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

# --- Link kernel ---
kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)

# --- ISO ---
$(ISO_OUT): kernel.bin
	mkdir -p $(ISO_DIR)/boot/grub
	cp kernel.bin $(ISO_DIR)/boot/kernel.bin
	grub-mkrescue -o $(ISO_OUT) $(ISO_DIR)

run: $(ISO_OUT)
	qemu-system-i386 -cdrom $(ISO_OUT) -no-reboot

clean:
	rm -f kernel.bin $(ISO_OUT)
	find src -name "*.o" -type f -delete

.PHONY: all run clean

