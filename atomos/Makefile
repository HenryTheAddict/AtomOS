# AtomOS - Main Makefile
# Builds the complete operating system and creates a bootable ISO

# Toolchain
CC = i686-elf-gcc
AS = nasm
LD = i686-elf-ld
OBJCOPY = i686-elf-objcopy

# Use system gcc if cross-compiler not available
ifeq ($(shell which $(CC) 2>/dev/null),)
    CC = gcc -m32 -ffreestanding
    LD = ld -m elf_i386
    OBJCOPY = objcopy
endif

# Directories
BOOT_DIR = boot
KERNEL_DIR = kernel
JAVIER_DIR = javier
COMPAT_DIR = compat
ISO_DIR = iso
BUILD_DIR = build

# Flags
CFLAGS = -std=c99 -ffreestanding -O2 -Wall -Wextra -Werror \
         -fno-stack-protector -fno-pic -fno-pie -nostdlib \
         -I$(KERNEL_DIR)/include
ASFLAGS = -f elf32
LDFLAGS = -T $(KERNEL_DIR)/linker.ld -nostdlib

# Source files
BOOT_SRC = $(BOOT_DIR)/boot.asm $(BOOT_DIR)/stage2.asm

KERNEL_ASM = $(KERNEL_DIR)/multiboot.asm \
             $(KERNEL_DIR)/arch/x86/gdt_asm.asm \
             $(KERNEL_DIR)/arch/x86/isr_asm.asm \
             $(KERNEL_DIR)/process/context_switch.asm

KERNEL_C = $(KERNEL_DIR)/main.c \
           $(KERNEL_DIR)/string.c \
           $(KERNEL_DIR)/libgcc.c \
           $(KERNEL_DIR)/arch/x86/gdt.c \
           $(KERNEL_DIR)/arch/x86/idt.c \
           $(KERNEL_DIR)/mm/pmm.c \
           $(KERNEL_DIR)/mm/vmm.c \
           $(KERNEL_DIR)/mm/heap.c \
           $(KERNEL_DIR)/drivers/video/vga.c \
           $(KERNEL_DIR)/drivers/video/framebuffer.c \
           $(KERNEL_DIR)/drivers/keyboard/keyboard.c \
           $(KERNEL_DIR)/drivers/timer.c \
           $(KERNEL_DIR)/fs/vfs.c \
           $(KERNEL_DIR)/fs/ramfs.c \
           $(KERNEL_DIR)/process/process.c \
           $(KERNEL_DIR)/syscall/syscall.c

JAVIER_C = $(JAVIER_DIR)/core/javier.c

COMPAT_C = $(COMPAT_DIR)/linux/linux_compat.c \
           $(COMPAT_DIR)/windows/windows_compat.c

# Object files
KERNEL_OBJ = $(patsubst %.c,$(BUILD_DIR)/%.o,$(KERNEL_C)) \
             $(patsubst %.c,$(BUILD_DIR)/%.o,$(JAVIER_C)) \
             $(patsubst %.c,$(BUILD_DIR)/%.o,$(COMPAT_C)) \
             $(patsubst %.asm,$(BUILD_DIR)/%.o,$(KERNEL_ASM))

# Output files
BOOTLOADER = $(BUILD_DIR)/boot.bin
STAGE2 = $(BUILD_DIR)/stage2.bin
KERNEL = $(BUILD_DIR)/kernel.bin
OS_IMAGE = $(BUILD_DIR)/atomos.img
ISO_IMAGE = $(BUILD_DIR)/atomos.iso

# Default target
.PHONY: all clean iso run debug

all: $(ISO_IMAGE)

# Create build directories
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/arch/x86
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/mm
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/drivers/video
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/drivers/keyboard
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/fs
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/process
	@mkdir -p $(BUILD_DIR)/$(KERNEL_DIR)/syscall
	@mkdir -p $(BUILD_DIR)/$(JAVIER_DIR)/core
	@mkdir -p $(BUILD_DIR)/$(COMPAT_DIR)/linux
	@mkdir -p $(BUILD_DIR)/$(COMPAT_DIR)/windows

# Compile bootloader
$(BUILD_DIR)/boot.bin: $(BOOT_DIR)/boot.asm | $(BUILD_DIR)
	@echo "  NASM    $<"
	@$(AS) -f bin $< -o $@

$(BUILD_DIR)/stage2.bin: $(BOOT_DIR)/stage2.asm | $(BUILD_DIR)
	@echo "  NASM    $<"
	@$(AS) -f bin $< -o $@

# Compile assembly files
$(BUILD_DIR)/%.o: %.asm | $(BUILD_DIR)
	@echo "  NASM    $<"
	@mkdir -p $(dir $@)
	@$(AS) $(ASFLAGS) $< -o $@

# Compile C files
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@echo "  CC      $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

# Link kernel
$(BUILD_DIR)/kernel.elf: $(KERNEL_OBJ)
	@echo "  LD      $@"
	@$(LD) $(LDFLAGS) -o $@ $^

$(KERNEL): $(BUILD_DIR)/kernel.elf
	@echo "  OBJCOPY $@"
	@$(OBJCOPY) -O binary $< $@

# Create disk image
$(OS_IMAGE): $(BUILD_DIR)/boot.bin $(BUILD_DIR)/stage2.bin $(KERNEL)
	@echo "  IMG     $@"
	@dd if=/dev/zero of=$@ bs=512 count=2880 2>/dev/null
	@dd if=$(BUILD_DIR)/boot.bin of=$@ bs=512 count=1 conv=notrunc 2>/dev/null
	@dd if=$(BUILD_DIR)/stage2.bin of=$@ bs=512 seek=1 count=8 conv=notrunc 2>/dev/null
	@dd if=$(KERNEL) of=$@ bs=512 seek=9 conv=notrunc 2>/dev/null

# Create ISO image with GRUB
$(ISO_IMAGE): $(BUILD_DIR)/kernel.elf
	@echo "  ISO     $@"
	@mkdir -p $(ISO_DIR)/boot/grub
	@cp $(BUILD_DIR)/kernel.elf $(ISO_DIR)/boot/atomos.elf
	@echo 'set timeout=3' > $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'set default=0' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'menuentry "AtomOS - Javier Desktop" {' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '    multiboot /boot/atomos.elf' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '    boot' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo 'menuentry "AtomOS - Safe Mode" {' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '    multiboot /boot/atomos.elf safemode' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '    boot' >> $(ISO_DIR)/boot/grub/grub.cfg
	@echo '}' >> $(ISO_DIR)/boot/grub/grub.cfg
	@grub-mkrescue -o $@ $(ISO_DIR) 2>/dev/null || \
	 (echo "Note: grub-mkrescue not available, using raw image" && cp $(OS_IMAGE) $@)

# Alternative ISO without GRUB (uses our bootloader directly)
iso-raw: $(OS_IMAGE)
	@echo "  ISO     $(ISO_IMAGE) (raw)"
	@cp $(OS_IMAGE) $(ISO_IMAGE)

# Run in QEMU
run: $(ISO_IMAGE)
	@echo "Starting AtomOS in QEMU..."
	qemu-system-i386 -cdrom $(ISO_IMAGE) -m 128M -serial stdio

# Run with floppy image
run-floppy: $(OS_IMAGE)
	@echo "Starting AtomOS in QEMU (floppy mode)..."
	qemu-system-i386 -fda $(OS_IMAGE) -m 128M -serial stdio

# Run with graphics
run-gui: $(ISO_IMAGE)
	@echo "Starting AtomOS with VGA..."
	qemu-system-i386 -cdrom $(ISO_IMAGE) -m 128M -vga std

# Debug in QEMU with GDB
debug: $(ISO_IMAGE)
	@echo "Starting AtomOS in debug mode..."
	@echo "Connect GDB with: target remote localhost:1234"
	qemu-system-i386 -cdrom $(ISO_IMAGE) -m 128M -s -S -serial stdio

# Clean build files
clean:
	@echo "  CLEAN"
	@rm -rf $(BUILD_DIR)
	@rm -rf $(ISO_DIR)/boot/atomos.img
	@rm -rf $(ISO_DIR)/boot/grub/grub.cfg

# Install dependencies (for Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install -y nasm gcc make qemu-system-i386 grub-pc-bin xorriso mtools

# Print info
info:
	@echo "AtomOS Build System"
	@echo "==================="
	@echo "CC       = $(CC)"
	@echo "AS       = $(AS)"
	@echo "LD       = $(LD)"
	@echo ""
	@echo "Targets:"
	@echo "  all        - Build everything and create ISO"
	@echo "  iso-raw    - Create raw bootable image"
	@echo "  run        - Run in QEMU"
	@echo "  run-gui    - Run with VGA display"
	@echo "  debug      - Run with GDB support"
	@echo "  clean      - Clean build files"
	@echo "  install-deps - Install required packages"
