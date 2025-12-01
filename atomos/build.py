#!/usr/bin/env python3
"""
AtomOS Build Script
Builds the complete operating system and creates a bootable ISO.

Usage:
    python build.py [options]

Options:
    --clean         Clean build directory before building
    --debug         Build with debug symbols
    --verbose       Show all compiler output
    --iso           Build ISO only (skip raw image)
    --help          Show this help message
"""

import os
import sys
import subprocess
import shutil
import argparse
import time
from pathlib import Path

# ANSI color codes
class Colors:
    HEADER = '\033[95m'
    BLUE = '\033[94m'
    CYAN = '\033[96m'
    GREEN = '\033[92m'
    YELLOW = '\033[93m'
    RED = '\033[91m'
    END = '\033[0m'
    BOLD = '\033[1m'

def print_header(msg):
    print(f"\n{Colors.HEADER}{Colors.BOLD}{'='*60}{Colors.END}")
    print(f"{Colors.HEADER}{Colors.BOLD}  {msg}{Colors.END}")
    print(f"{Colors.HEADER}{Colors.BOLD}{'='*60}{Colors.END}\n")

def print_step(msg):
    print(f"{Colors.CYAN}[BUILD]{Colors.END} {msg}")

def print_success(msg):
    print(f"{Colors.GREEN}[SUCCESS]{Colors.END} {msg}")

def print_warning(msg):
    print(f"{Colors.YELLOW}[WARNING]{Colors.END} {msg}")

def print_error(msg):
    print(f"{Colors.RED}[ERROR]{Colors.END} {msg}")

def check_dependencies():
    """Check for required build tools."""
    print_step("Checking build dependencies...")
    
    required = {
        'nasm': 'NASM assembler',
        'make': 'GNU Make',
    }
    
    # Check for cross-compiler or system gcc
    has_cross = shutil.which('i686-elf-gcc') is not None
    has_gcc = shutil.which('gcc') is not None
    
    missing = []
    for cmd, name in required.items():
        if not shutil.which(cmd):
            missing.append(f"{name} ({cmd})")
    
    if not has_cross and not has_gcc:
        missing.append("GCC compiler (gcc or i686-elf-gcc)")
    
    if missing:
        print_error("Missing required dependencies:")
        for dep in missing:
            print(f"  - {dep}")
        print("\nInstall on Ubuntu/Debian:")
        print("  sudo apt-get install nasm gcc make grub-pc-bin xorriso mtools")
        return False
    
    if has_cross:
        print_success("Found i686-elf cross-compiler toolchain")
    else:
        print_warning("Using system GCC (cross-compiler recommended)")
    
    # Check for optional tools
    optional = {
        'grub-mkrescue': 'GRUB (for ISO creation)',
        'xorriso': 'xorriso (for ISO creation)',
        'qemu-system-i386': 'QEMU (for testing)',
    }
    
    for cmd, name in optional.items():
        if shutil.which(cmd):
            print_success(f"Found {name}")
        else:
            print_warning(f"Optional: {name} not found")
    
    return True

def clean_build(build_dir):
    """Clean the build directory."""
    print_step(f"Cleaning build directory: {build_dir}")
    if os.path.exists(build_dir):
        shutil.rmtree(build_dir)
    print_success("Build directory cleaned")

def run_make(args, verbose=False):
    """Run make with given arguments."""
    cmd = ['make'] + args
    
    if verbose:
        result = subprocess.run(cmd, cwd=os.path.dirname(os.path.abspath(__file__)))
    else:
        result = subprocess.run(
            cmd, 
            cwd=os.path.dirname(os.path.abspath(__file__)),
            capture_output=True,
            text=True
        )
        if result.returncode != 0:
            print(result.stdout)
            print(result.stderr)
    
    return result.returncode == 0

def build_atomos(clean=False, debug=False, verbose=False, iso_only=False):
    """Build AtomOS."""
    start_time = time.time()
    
    print_header("AtomOS Build System")
    
    # Get script directory
    script_dir = Path(__file__).parent.resolve()
    os.chdir(script_dir)
    
    build_dir = script_dir / 'build'
    
    # Check dependencies
    if not check_dependencies():
        return False
    
    # Clean if requested
    if clean:
        clean_build(build_dir)
    
    # Build
    print_step("Building AtomOS...")
    
    make_args = []
    if verbose:
        make_args.append('V=1')
    
    if not run_make(make_args, verbose):
        print_error("Build failed!")
        return False
    
    # Check outputs
    kernel_elf = build_dir / 'kernel.elf'
    iso_file = build_dir / 'atomos.iso'
    
    if kernel_elf.exists():
        size = kernel_elf.stat().st_size
        print_success(f"Kernel built: {kernel_elf} ({size:,} bytes)")
    
    if iso_file.exists():
        size = iso_file.stat().st_size
        print_success(f"ISO created: {iso_file} ({size:,} bytes)")
    else:
        print_warning("ISO not created (grub-mkrescue may not be available)")
    
    # Print build time
    elapsed = time.time() - start_time
    print(f"\n{Colors.GREEN}Build completed in {elapsed:.2f} seconds{Colors.END}")
    
    # Print next steps
    print(f"\n{Colors.CYAN}Next steps:{Colors.END}")
    print(f"  Run in QEMU:     python emu.py")
    print(f"  Run with GUI:    python emu.py --gui")
    print(f"  Debug mode:      python emu.py --debug")
    
    return True

def main():
    parser = argparse.ArgumentParser(
        description='AtomOS Build Script',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  python build.py              # Standard build
  python build.py --clean      # Clean and rebuild
  python build.py --verbose    # Show all output
  python build.py --debug      # Build with debug symbols
        '''
    )
    
    parser.add_argument('--clean', '-c', action='store_true',
                        help='Clean build directory before building')
    parser.add_argument('--debug', '-d', action='store_true',
                        help='Build with debug symbols')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='Show all compiler output')
    parser.add_argument('--iso', action='store_true',
                        help='Build ISO only')
    
    args = parser.parse_args()
    
    success = build_atomos(
        clean=args.clean,
        debug=args.debug,
        verbose=args.verbose,
        iso_only=args.iso
    )
    
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
