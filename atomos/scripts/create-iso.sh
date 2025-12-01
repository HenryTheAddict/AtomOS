#!/bin/bash
#
# AtomOS ISO Creation Script
# Creates a bootable ISO image from the built OS
#

set -e

# Configuration
BUILD_DIR="build"
ISO_DIR="iso"
ISO_NAME="atomos.iso"
BOOT_IMAGE="atomos.img"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}"
echo "╔═══════════════════════════════════════════════════════════╗"
echo "║              AtomOS ISO Creation Script                   ║"
echo "╚═══════════════════════════════════════════════════════════╝"
echo -e "${NC}"

# Check if build directory exists
if [ ! -d "$BUILD_DIR" ]; then
    echo -e "${RED}Error: Build directory not found. Run 'make' first.${NC}"
    exit 1
fi

# Check if boot image exists
if [ ! -f "$BUILD_DIR/$BOOT_IMAGE" ]; then
    echo -e "${RED}Error: Boot image not found at $BUILD_DIR/$BOOT_IMAGE${NC}"
    echo "Run 'make' to build the OS first."
    exit 1
fi

# Create ISO directory structure
echo "Creating ISO directory structure..."
mkdir -p "$ISO_DIR/boot/grub"

# Copy boot image
echo "Copying boot image..."
cp "$BUILD_DIR/$BOOT_IMAGE" "$ISO_DIR/boot/"

# Create GRUB configuration
echo "Creating GRUB configuration..."
cat > "$ISO_DIR/boot/grub/grub.cfg" << 'EOF'
# AtomOS GRUB Configuration

set timeout=3
set default=0

# Boot menu styling
set menu_color_normal=light-blue/black
set menu_color_highlight=light-cyan/blue

menuentry "AtomOS - Javier Desktop" {
    multiboot /boot/atomos.img
    boot
}

menuentry "AtomOS - Safe Mode" {
    multiboot /boot/atomos.img safemode
    boot
}

menuentry "AtomOS - Debug Mode" {
    multiboot /boot/atomos.img debug
    boot
}
EOF

# Check for grub-mkrescue
if command -v grub-mkrescue &> /dev/null; then
    echo "Creating ISO with GRUB..."
    grub-mkrescue -o "$BUILD_DIR/$ISO_NAME" "$ISO_DIR" 2>/dev/null
    
    if [ $? -eq 0 ]; then
        echo -e "${GREEN}✓ ISO created successfully: $BUILD_DIR/$ISO_NAME${NC}"
        ls -lh "$BUILD_DIR/$ISO_NAME"
    else
        echo -e "${YELLOW}Warning: grub-mkrescue failed, creating raw image...${NC}"
        cp "$BUILD_DIR/$BOOT_IMAGE" "$BUILD_DIR/$ISO_NAME"
    fi
else
    echo -e "${YELLOW}grub-mkrescue not found, creating raw bootable image...${NC}"
    cp "$BUILD_DIR/$BOOT_IMAGE" "$BUILD_DIR/$ISO_NAME"
fi

echo ""
echo -e "${GREEN}Build complete!${NC}"
echo ""
echo "To run AtomOS:"
echo "  1. In QEMU:    qemu-system-i386 -cdrom $BUILD_DIR/$ISO_NAME -m 128M"
echo "  2. In VirtualBox: Create new VM and attach $BUILD_DIR/$ISO_NAME"
echo "  3. On real hardware: Burn to CD/DVD or create bootable USB"
echo ""
echo "To create a bootable USB (be careful with device selection!):"
echo "  sudo dd if=$BUILD_DIR/$ISO_NAME of=/dev/sdX bs=4M status=progress"
echo ""
