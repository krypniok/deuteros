# $@ = target file
# $< = first dependency
# $^ = all dependencies

# detect all .o files based on their .c source
C_SOURCES = $(wildcard kernel/*.c drivers/*.c cpu/*.c stdlibs/*.c programs/*.c programs/dobby/*.c)
HEADERS = $(wildcard kernel/*.h  drivers/*.h cpu/*.h stdlibs/*.h programs/*.h programs//dobby/*.h)
OBJ_FILES = ${C_SOURCES:.c=.o cpu/interrupt.o cpu/setjmp.o cpu/longjmp.o}

# First rule is the one executed when no parameters are fed to the Makefile
all: run

# Read current revision number from file
CURRENT_REVISION := $(shell cat version.txt)
CURRENT_DATE := $(shell date)

# Increment the revision number
NEW_REVISION := $(shell expr $(CURRENT_REVISION) + 1)

# Update the version file with the new revision number
$(shell echo $(NEW_REVISION) > version.txt)

# Define revision number as a preprocessor macro
CFLAGS := -DREVISION_NUMBER=$(NEW_REVISION) -DREVISION_DATE='"$(CURRENT_DATE)"'

# Notice how dependencies are built as needed
kernel.bin: boot/kernel_entry.o ${OBJ_FILES}
	ld -nostdlib -m elf_i386 -o $@ -Ttext 0x10000 $^ --oformat binary

os-image.bin: boot/mbr.bin kernel.bin
	cat $^ > $@

run: os-image.bin
	dd if=/dev/zero of=disk_image.img bs=512 count=2880
	dd if=os-image.bin of=disk_image.img conv=notrunc
	dd if=test.pc of=disk_image.img bs=1 seek=1048576 conv=notrunc
	qemu-system-i386 -m 1024 -vga std -rtc "base=2023-08-03T12:34:56" -soundhw pcspk -device sb16 -drive format=raw,file=disk_image.img
	dd if=disk_image.img of=test.pc bs=1 skip=1048576 count=1024

rerun:
	qemu-system-i386 -vga std -rtc "base=2023-08-03T12:34:56" -soundhw pcspk -device sb16 -drive format=raw,file=disk_image.img

echo: os-image.bin
	xxd $<

# only for debug
kernel.elf: boot/kernel_entry.o ${OBJ_FILES}
	ld -nostdlib -m elf_i386 -o $@ -Ttext 0x10000 $^

debug: os-image.bin kernel.elf
	qemu-system-i386 -s -S -fda disk_image.img -d guest_errors,int &
	gdb -ex "target remote localhost:1234" -ex "symbol-file kernel.elf"

%.o: %.c ${HEADERS}
	gcc -Wno-implicit-function-declaration -Wno-overflow -fno-PIC -nostdlib --no-pie -m32 -ffreestanding $(CFLAGS) -c $< -o $@

%.o: %.asm
	nasm $< -f elf -o $@

%.bin: %.asm
	nasm $< -f bin -o $@

%.dis: %.bin
	ndisasm -b 32 $< > $@

clean:
	$(RM) *.bin *.o *.dis *.elf
	$(RM) kernel/*.o
	$(RM) boot/*.o boot/*.bin
	$(RM) drivers/*.o
	$(RM) cpu/*.o
	$(RM) programs/*.o
	$(RM) stdlibs/*.o
