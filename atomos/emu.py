#!/usr/bin/env python3
"""
AtomOS QEMU Emulator Script
Auto-configures and runs AtomOS in QEMU with optimal settings.

Usage:
    python emu.py [options]

Options:
    --gui           Run with graphical display (default: serial console)
    --debug         Enable GDB debugging (port 1234)
    --memory SIZE   Set RAM size (default: 128M)
    --cores N       Set CPU cores (default: 1)
    --audio         Enable audio output
    --network       Enable network support
    --floppy        Boot from floppy image instead of ISO
    --build         Build before running
    --help          Show this help message
"""

import os
import sys
import subprocess
import shutil
import argparse
import platform
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

def print_info(msg):
    print(f"{Colors.CYAN}[INFO]{Colors.END} {msg}")

def print_success(msg):
    print(f"{Colors.GREEN}[OK]{Colors.END} {msg}")

def print_warning(msg):
    print(f"{Colors.YELLOW}[WARN]{Colors.END} {msg}")

def print_error(msg):
    print(f"{Colors.RED}[ERROR]{Colors.END} {msg}")

def find_qemu():
    """Find the appropriate QEMU binary."""
    # List of possible QEMU binary names
    qemu_names = [
        'qemu-system-i386',
        'qemu-system-x86_64',
        'qemu-system-i686',
    ]
    
    for name in qemu_names:
        path = shutil.which(name)
        if path:
            return path
    
    return None

def get_qemu_version(qemu_path):
    """Get QEMU version."""
    try:
        result = subprocess.run(
            [qemu_path, '--version'],
            capture_output=True,
            text=True
        )
        # Extract version from first line
        first_line = result.stdout.split('\n')[0]
        return first_line
    except:
        return "Unknown"

def detect_kvm():
    """Check if KVM is available."""
    if platform.system() != 'Linux':
        return False
    
    # Check for /dev/kvm
    if os.path.exists('/dev/kvm'):
        # Check if accessible
        return os.access('/dev/kvm', os.R_OK | os.W_OK)
    
    return False

def build_atomos():
    """Build AtomOS using build.py."""
    script_dir = Path(__file__).parent.resolve()
    build_script = script_dir / 'build.py'
    
    if not build_script.exists():
        print_error("build.py not found!")
        return False
    
    print_info("Building AtomOS...")
    result = subprocess.run([sys.executable, str(build_script)], cwd=script_dir)
    return result.returncode == 0

def run_qemu(args):
    """Run QEMU with the specified configuration."""
    script_dir = Path(__file__).parent.resolve()
    build_dir = script_dir / 'build'
    
    # Find boot media
    iso_path = build_dir / 'atomos.iso'
    img_path = build_dir / 'atomos.img'
    
    if args.floppy:
        boot_media = img_path
        boot_type = 'floppy'
    else:
        boot_media = iso_path
        boot_type = 'cdrom'
    
    if not boot_media.exists():
        print_error(f"Boot media not found: {boot_media}")
        print_info("Run 'python build.py' first or use --build flag")
        return False
    
    # Find QEMU
    qemu_path = find_qemu()
    if not qemu_path:
        print_error("QEMU not found!")
        print_info("Install QEMU:")
        print("  Ubuntu/Debian: sudo apt-get install qemu-system-x86")
        print("  macOS:         brew install qemu")
        print("  Windows:       Download from https://www.qemu.org/download/")
        return False
    
    print_header("AtomOS QEMU Launcher")
    print_info(f"QEMU: {qemu_path}")
    print_info(f"Version: {get_qemu_version(qemu_path)}")
    print_info(f"Boot media: {boot_media} ({boot_type})")
    print_info(f"Memory: {args.memory}")
    print_info(f"CPU cores: {args.cores}")
    
    # Build QEMU command
    cmd = [qemu_path]
    
    # Memory
    cmd.extend(['-m', args.memory])
    
    # CPU
    cmd.extend(['-smp', str(args.cores)])
    
    # Boot media
    if boot_type == 'cdrom':
        cmd.extend(['-cdrom', str(boot_media)])
    else:
        cmd.extend(['-fda', str(boot_media)])
    
    # Display settings
    if args.gui:
        cmd.extend(['-vga', 'std'])
        print_info("Display: VGA (graphical)")
    else:
        cmd.extend(['-nographic'])
        cmd.extend(['-serial', 'mon:stdio'])
        print_info("Display: Serial console (text mode)")
        print_info("Press Ctrl+A, X to exit")
    
    # KVM acceleration
    if detect_kvm() and not args.debug:
        cmd.extend(['-enable-kvm'])
        print_success("KVM acceleration enabled")
    else:
        if platform.system() == 'Linux':
            print_warning("KVM not available, using software emulation")
    
    # Audio
    if args.audio:
        # Try different audio backends
        if platform.system() == 'Linux':
            cmd.extend(['-audiodev', 'pa,id=audio0', '-machine', 'pcspk-audiodev=audio0'])
        elif platform.system() == 'Darwin':
            cmd.extend(['-audiodev', 'coreaudio,id=audio0', '-machine', 'pcspk-audiodev=audio0'])
        else:
            cmd.extend(['-soundhw', 'pcspk'])
        print_info("Audio: PC Speaker enabled")
    
    # Network
    if args.network:
        cmd.extend(['-netdev', 'user,id=net0', '-device', 'e1000,netdev=net0'])
        print_info("Network: User-mode networking enabled")
    
    # Debug mode
    if args.debug:
        cmd.extend(['-s', '-S'])
        print_info("Debug: GDB server on localhost:1234")
        print_info("Connect with: gdb -ex 'target remote localhost:1234'")
    
    # Boot order
    if boot_type == 'cdrom':
        cmd.extend(['-boot', 'd'])
    else:
        cmd.extend(['-boot', 'a'])
    
    # Additional options for stability
    cmd.extend(['-no-reboot'])  # Don't reboot on triple fault
    cmd.extend(['-no-shutdown'])  # Don't shutdown on error
    
    print(f"\n{Colors.GREEN}Starting AtomOS...{Colors.END}\n")
    print(f"{Colors.CYAN}Command: {' '.join(cmd)}{Colors.END}\n")
    
    if not args.gui:
        print("-" * 60)
        print("AtomOS Serial Console")
        print("Press Ctrl+A, X to exit QEMU")
        print("-" * 60 + "\n")
    
    # Run QEMU
    try:
        result = subprocess.run(cmd, cwd=script_dir)
        return result.returncode == 0
    except KeyboardInterrupt:
        print("\n\nQEMU terminated by user")
        return True

def main():
    parser = argparse.ArgumentParser(
        description='AtomOS QEMU Emulator Script',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog='''
Examples:
  python emu.py                # Run with serial console
  python emu.py --gui          # Run with graphical display
  python emu.py --debug        # Run with GDB debugging
  python emu.py --build --gui  # Build and run with GUI
  python emu.py -m 256M -c 2   # 256MB RAM, 2 cores

Keyboard shortcuts (serial mode):
  Ctrl+A, X     Exit QEMU
  Ctrl+A, H     Help
  Ctrl+A, C     Switch to QEMU monitor
        '''
    )
    
    parser.add_argument('--gui', '-g', action='store_true',
                        help='Run with graphical display')
    parser.add_argument('--debug', '-d', action='store_true',
                        help='Enable GDB debugging on port 1234')
    parser.add_argument('--memory', '-m', default='128M',
                        help='Set RAM size (default: 128M)')
    parser.add_argument('--cores', '-c', type=int, default=1,
                        help='Set CPU cores (default: 1)')
    parser.add_argument('--audio', '-a', action='store_true',
                        help='Enable audio output (PC Speaker)')
    parser.add_argument('--network', '-n', action='store_true',
                        help='Enable network support')
    parser.add_argument('--floppy', '-f', action='store_true',
                        help='Boot from floppy image instead of ISO')
    parser.add_argument('--build', '-b', action='store_true',
                        help='Build AtomOS before running')
    
    args = parser.parse_args()
    
    # Build if requested
    if args.build:
        if not build_atomos():
            print_error("Build failed!")
            sys.exit(1)
    
    # Run QEMU
    success = run_qemu(args)
    sys.exit(0 if success else 1)

if __name__ == '__main__':
    main()
