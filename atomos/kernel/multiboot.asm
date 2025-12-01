; AtomOS - Multiboot Entry Point
; Compatible with GRUB and multiboot specification

[BITS 32]

; Multiboot constants
MBOOT_PAGE_ALIGN    equ 1 << 0
MBOOT_MEM_INFO      equ 1 << 1
MBOOT_VIDEO_MODE    equ 1 << 2
MBOOT_HEADER_MAGIC  equ 0x1BADB002
MBOOT_HEADER_FLAGS  equ MBOOT_PAGE_ALIGN | MBOOT_MEM_INFO | MBOOT_VIDEO_MODE
MBOOT_CHECKSUM      equ -(MBOOT_HEADER_MAGIC + MBOOT_HEADER_FLAGS)

; Video mode request
VIDEO_MODE          equ 0       ; 0 = linear framebuffer
VIDEO_WIDTH         equ 1024
VIDEO_HEIGHT        equ 768
VIDEO_DEPTH         equ 32

; Kernel stack size
KERNEL_STACK_SIZE   equ 0x4000  ; 16KB

section .multiboot
align 4
    ; Multiboot header
    dd MBOOT_HEADER_MAGIC
    dd MBOOT_HEADER_FLAGS
    dd MBOOT_CHECKSUM
    
    ; AOUT kludge (not used, but required)
    dd 0    ; header_addr
    dd 0    ; load_addr
    dd 0    ; load_end_addr
    dd 0    ; bss_end_addr
    dd 0    ; entry_addr
    
    ; Video mode
    dd VIDEO_MODE
    dd VIDEO_WIDTH
    dd VIDEO_HEIGHT
    dd VIDEO_DEPTH

section .bss
align 16
stack_bottom:
    resb KERNEL_STACK_SIZE
stack_top:

section .text
global _start
extern kernel_main

_start:
    ; Disable interrupts
    cli
    
    ; Setup stack
    mov esp, stack_top
    
    ; Push multiboot info pointer and magic
    push ebx        ; Multiboot info structure pointer
    push eax        ; Multiboot magic number (should be 0x2BADB002)
    
    ; Create boot_info structure on stack
    sub esp, 32
    mov edi, esp
    
    ; Check multiboot magic
    cmp eax, 0x2BADB002
    jne .no_multiboot
    
    ; Parse multiboot info structure
    mov esi, ebx
    
    ; Get memory size (lower + upper)
    mov eax, [esi + 4]      ; mem_lower (KB)
    mov edx, [esi + 8]      ; mem_upper (KB)
    add eax, edx
    shl eax, 10             ; Convert to bytes
    mov [edi + 28], eax     ; boot_info->total_memory
    
    ; Get memory map
    mov eax, [esi + 44]     ; mmap_length
    mov [edi + 0], eax      ; boot_info->memory_map_entries
    mov eax, [esi + 48]     ; mmap_addr
    mov [edi + 4], eax      ; boot_info->memory_map_addr
    
    ; Get framebuffer info
    test dword [esi], 0x800 ; Check if framebuffer info present
    jz .no_framebuffer
    
    mov eax, [esi + 88]     ; framebuffer_addr
    mov [edi + 8], eax      ; boot_info->framebuffer_addr
    mov eax, [esi + 92]     ; framebuffer_width
    mov [edi + 12], eax     ; boot_info->screen_width
    mov eax, [esi + 96]     ; framebuffer_height
    mov [edi + 16], eax     ; boot_info->screen_height
    mov eax, [esi + 100]    ; framebuffer_bpp
    and eax, 0xFF
    mov [edi + 20], eax     ; boot_info->bits_per_pixel
    mov eax, [esi + 104]    ; framebuffer_pitch
    mov [edi + 24], eax     ; boot_info->pitch
    jmp .call_kernel

.no_framebuffer:
    ; No framebuffer - use VGA text mode
    mov dword [edi + 8], 0
    mov dword [edi + 12], 0
    mov dword [edi + 16], 0
    mov dword [edi + 20], 0
    mov dword [edi + 24], 0
    jmp .call_kernel

.no_multiboot:
    ; Not booted via multiboot - use defaults
    mov dword [edi + 0], 0      ; No memory map
    mov dword [edi + 4], 0
    mov dword [edi + 8], 0      ; No framebuffer
    mov dword [edi + 12], 0
    mov dword [edi + 16], 0
    mov dword [edi + 20], 0
    mov dword [edi + 24], 0
    mov dword [edi + 28], 0x4000000  ; Assume 64MB

.call_kernel:
    ; Call kernel_main with boot_info pointer
    push edi
    call kernel_main
    
    ; Should never return, but just in case
    cli
.hang:
    hlt
    jmp .hang
