

CC = gcc
LD = ld
AS = nasm



CFLAGS  = -ffreestanding -O2 -Wall -Wextra -m32 \
          -fno-pic -fno-pie -fno-stack-protector \
          -nostdlib -nostartfiles -nodefaultlibs

LDFLAGS = -T linker.ld -nostdlib -m elf_i386



ISO_DIR = iso
ISO_OUT = myos.iso



OBJS = \
  src/boot/boot.o \
  src/kernel/kernel.o \
  src/drivers/vga.o \
  src/drivers/pic.o \
  src/drivers/pit.o \
  src/drivers/keyboard.o \
  src/drivers/ata.o \
  src/cpu/idt.o \
  src/cpu/irq.o \
  src/cpu/isr.o \
  src/cpu/gdt.o \
  src/cpu/gdt_flush.o \
  src/util/string.o \
  src/fs/fs.o \
  src/editor/editor.o



all: $(ISO_OUT)



src/boot/boot.o: src/boot/boot.s
	$(AS) -f elf32 $< -o $@

src/cpu/isr.o: src/cpu/isr.s
	$(AS) -f elf32 $< -o $@

src/cpu/gdt_flush.o: src/cpu/gdt_flush.s
	$(AS) -f elf32 $< -o $@



src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@



kernel.bin: $(OBJS)
	$(LD) $(LDFLAGS) -o $@ $(OBJS)



$(ISO_OUT): kernel.bin
	mkdir -p $(ISO_DIR)/boot/grub
	cp kernel.bin $(ISO_DIR)/boot/kernel.bin
	echo 'set timeout=0' > $(ISO_DIR)/boot/grub/grub.cfg
	echo 'set default=0' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo 'menuentry "xyuOS" {' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '  multiboot /boot/kernel.bin' >> $(ISO_DIR)/boot/grub/grub.cfg
	echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO_OUT) $(ISO_DIR) >/dev/null 2>&1



run: $(ISO_OUT)
	qemu-system-i386 \
		-cdrom $(ISO_OUT) \
		-drive format=raw,file=disk.img,if=ide,index=0 \
		-no-reboot



clean:
	rm -f kernel.bin $(ISO_OUT)
	find src -name "*.o" -type f -delete

.PHONY: all run clean
