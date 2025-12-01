# AtomOS

<p align="center">
  <img src="https://img.shields.io/badge/version-1.0.0-blue.svg" alt="Version">
  <img src="https://img.shields.io/badge/architecture-x86-green.svg" alt="Architecture">
  <img src="https://img.shields.io/badge/license-MIT-yellow.svg" alt="License">
</p>

**AtomOS** is a clean, modern operating system built from scratch in pure C. It features the **Javier** desktop environment - a sleek, minimalist graphical interface designed for simplicity and elegance.

## ✨ Features

### Core System
- **Custom Kernel**: Written entirely in C and x86 assembly, no external dependencies
- **Memory Management**: Physical and virtual memory managers with paging support
- **Process Management**: Multitasking with preemptive scheduling
- **Virtual File System**: Unified file system interface with RAM filesystem
- **System Calls**: Linux-compatible syscall interface

### Javier Desktop Environment
- **Modern UI**: Clean, dark-themed interface with smooth gradients
- **Window Manager**: Draggable, resizable windows with shadows
- **Widget Toolkit**: Labels, buttons, text boxes, checkboxes, and more
- **Taskbar**: Application launcher and window management
- **Compositing**: Double-buffered rendering for flicker-free display

### Application Compatibility
- **Linux Binary Support**: ELF executable loader with syscall emulation
- **Windows Binary Support**: PE executable loader with Win32 API emulation

## 🏗️ Architecture

```
AtomOS/
├── boot/           # Stage 1 & 2 bootloaders
├── kernel/         # Core kernel
│   ├── arch/x86/   # x86-specific code (GDT, IDT, etc.)
│   ├── mm/         # Memory management
│   ├── fs/         # File systems
│   ├── process/    # Process management
│   ├── syscall/    # System call interface
│   └── drivers/    # Device drivers
├── javier/         # Javier desktop environment
│   ├── core/       # Window manager & compositor
│   ├── widgets/    # UI widgets
│   └── themes/     # Visual themes
├── compat/         # Compatibility layers
│   ├── linux/      # Linux ELF support
│   └── windows/    # Windows PE support
├── libc/           # C standard library
└── apps/           # Built-in applications
```

## 🚀 Building

### Prerequisites

Install the required build tools:

```bash
# Ubuntu/Debian
sudo apt-get install nasm gcc make qemu-system-i386 grub-pc-bin xorriso

# Or use the provided script
make install-deps
```

### Build

```bash
# Build everything
make

# Build and run in QEMU
make run

# Build with graphics
make run-gui

# Clean build files
make clean
```

### Output Files

- `build/atomos.img` - Raw bootable disk image
- `build/atomos.iso` - Bootable ISO (GRUB or raw)
- `build/kernel.bin` - Compiled kernel binary

## 💻 Running

### In QEMU (Recommended)

```bash
# CD-ROM mode
qemu-system-i386 -cdrom build/atomos.iso -m 128M

# Floppy mode
qemu-system-i386 -fda build/atomos.img -m 128M

# With VGA graphics
qemu-system-i386 -cdrom build/atomos.iso -m 128M -vga std

# Debug mode (connect with GDB)
qemu-system-i386 -cdrom build/atomos.iso -m 128M -s -S
```

### In VirtualBox

1. Create a new virtual machine
2. Type: Other, Version: Other/Unknown
3. RAM: 128 MB or more
4. Attach `build/atomos.iso` as CD/DVD
5. Boot!

### On Real Hardware

```bash
# Create bootable USB (DANGEROUS - double-check device!)
sudo dd if=build/atomos.iso of=/dev/sdX bs=4M status=progress
```

## 🎨 Javier UI

The Javier desktop environment features a modern, dark theme:

| Color | Hex | Usage |
|-------|-----|-------|
| Background Dark | `#121218` | Desktop background |
| Background Light | `#1C1C26` | Window backgrounds |
| Accent | `#58A6FF` | Highlights, buttons |
| Text | `#E6E6F0` | Primary text |
| Border | `#303044` | Window borders |

### Creating Windows

```c
// Create a window
javier_window_t *win = javier_window_create(
    "My App",           // Title
    100, 100,           // Position
    400, 300,           // Size
    WIN_DEFAULT         // Flags
);

// Add widgets
javier_label_create(win, "Hello!", 20, 20, 100, 20);
javier_button_create(win, "Click Me", 20, 50, 100, 30);
```

## 🔧 System Calls

AtomOS supports Linux-compatible system calls:

| Syscall | Number | Description |
|---------|--------|-------------|
| exit | 1 | Terminate process |
| fork | 2 | Create child process |
| read | 3 | Read from file |
| write | 4 | Write to file |
| open | 5 | Open file |
| close | 6 | Close file |
| brk | 45 | Adjust heap |
| mmap | 90 | Map memory |
| ... | ... | And more! |

## 📁 File System

AtomOS uses a RAM-based file system with standard Unix directories:

```
/
├── bin/     # Executables
├── dev/     # Device files
├── etc/     # Configuration
├── home/    # User directories
├── tmp/     # Temporary files
├── usr/     # User programs
└── var/     # Variable data
```

## 🤝 Contributing

Contributions are welcome! Here's how to help:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

### Development Tips

- Use `make debug` to run with GDB support
- Check `kernel/include/kernel.h` for logging functions
- Follow the existing code style

## 📄 License

AtomOS is released under the MIT License. See [LICENSE](LICENSE) for details.

## 🙏 Acknowledgments

- OSDev Wiki for extensive documentation
- Intel Software Developer Manuals
- The open-source OS development community

---

<p align="center">
  <strong>AtomOS</strong> - A clean OS built from atoms of code<br>
  <em>Powered by the Javier Desktop Environment</em>
</p>
