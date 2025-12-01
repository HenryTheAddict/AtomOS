; AtomOS Stage 2 Bootloader
; Switches to protected mode, enables A20, sets up paging, and jumps to kernel

[BITS 16]
[ORG 0x10000]

KERNEL_PHYS_ADDR equ 0x20000
KERNEL_VIRT_ADDR equ 0x100000

start:
    ; Print message
    mov si, msg_stage2
    call print_string16

    ; Get memory map
    call get_memory_map

    ; Enable A20 line
    call enable_a20

    ; Load GDT
    lgdt [gdt_descriptor]

    ; Enable protected mode
    cli
    mov eax, cr0
    or eax, 1
    mov cr0, eax

    ; Far jump to 32-bit code
    jmp 0x08:protected_mode

; ============================================
; 16-bit functions
; ============================================

print_string16:
    pusha
.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    int 0x10
    jmp .loop
.done:
    popa
    ret

enable_a20:
    ; Try BIOS method first
    mov ax, 0x2401
    int 0x15
    jnc .done

    ; Fast A20 method
    in al, 0x92
    or al, 2
    out 0x92, al

.done:
    ret

; Get memory map using BIOS E820
get_memory_map:
    mov di, memory_map
    xor ebx, ebx
    mov edx, 0x534D4150     ; "SMAP"
.loop:
    mov eax, 0xE820
    mov ecx, 24
    int 0x15
    jc .done
    add di, 24
    inc byte [memory_map_entries]
    test ebx, ebx
    jnz .loop
.done:
    ret

; ============================================
; 32-bit protected mode
; ============================================

[BITS 32]
protected_mode:
    ; Setup segment registers
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000

    ; Print message (VGA text mode)
    mov esi, msg_protected
    mov edi, 0xB8000 + 160
    call print_string32

    ; Copy kernel to 1MB mark
    mov esi, KERNEL_PHYS_ADDR
    mov edi, KERNEL_VIRT_ADDR
    mov ecx, 0x8000         ; 32KB
    rep movsb

    ; Setup basic paging (identity map first 4MB + higher-half)
    call setup_paging

    ; Pass boot info to kernel
    mov eax, boot_info
    push eax

    ; Jump to kernel
    mov eax, KERNEL_VIRT_ADDR
    call eax

    ; Should never reach here
    jmp $

print_string32:
    push eax
.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0F            ; White on black
    stosw
    jmp .loop
.done:
    pop eax
    ret

setup_paging:
    ; Clear page directory
    mov edi, 0x1000         ; Page directory at 0x1000
    xor eax, eax
    mov ecx, 1024
    rep stosd

    ; Clear page table
    mov edi, 0x2000         ; Page table at 0x2000
    mov ecx, 1024
    rep stosd

    ; Identity map first 4MB
    mov edi, 0x2000
    mov eax, 0x00000003     ; Present, writable
    mov ecx, 1024
.fill_pt:
    stosd
    add eax, 0x1000
    loop .fill_pt

    ; Set page directory entry
    mov dword [0x1000], 0x2003      ; Point to page table

    ; Enable paging
    mov eax, 0x1000
    mov cr3, eax
    mov eax, cr0
    or eax, 0x80000000
    mov cr0, eax

    ret

; ============================================
; GDT
; ============================================

gdt_start:
    ; Null descriptor
    dq 0

    ; Code segment (0x08)
    dw 0xFFFF               ; Limit low
    dw 0x0000               ; Base low
    db 0x00                 ; Base middle
    db 10011010b            ; Access: present, ring 0, code, executable, readable
    db 11001111b            ; Flags: 4KB granularity, 32-bit, limit high
    db 0x00                 ; Base high

    ; Data segment (0x10)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b            ; Access: present, ring 0, data, writable
    db 11001111b
    db 0x00

    ; User code segment (0x18)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 11111010b            ; Ring 3 code
    db 11001111b
    db 0x00

    ; User data segment (0x20)
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 11110010b            ; Ring 3 data
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

; ============================================
; Data
; ============================================

msg_stage2:    db 'Stage 2: Entering protected mode...', 13, 10, 0
msg_protected: db 'Protected mode enabled! Starting AtomOS kernel...', 0

memory_map_entries: db 0

align 8
boot_info:
    dd memory_map_entries   ; Pointer to entry count
    dd memory_map           ; Pointer to memory map
    dd 0                    ; Framebuffer address (to be filled)
    dd 0                    ; Screen width
    dd 0                    ; Screen height
    dd 0                    ; Bits per pixel

memory_map:
    times 24 * 32 db 0      ; Space for 32 memory map entries

; Padding
times 4096 - ($ - $$) db 0
