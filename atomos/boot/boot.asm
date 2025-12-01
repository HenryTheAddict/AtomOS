; AtomOS Stage 1 Bootloader
; First 512 bytes loaded by BIOS at 0x7C00
; Loads Stage 2 bootloader and kernel

[BITS 16]
[ORG 0x7C00]

; Constants
STAGE2_SEGMENT  equ 0x1000
STAGE2_OFFSET   equ 0x0000
KERNEL_SEGMENT  equ 0x2000
KERNEL_OFFSET   equ 0x0000

start:
    ; Setup segments
    cli
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    sti

    ; Save boot drive
    mov [boot_drive], dl

    ; Print welcome message
    mov si, msg_boot
    call print_string

    ; Load Stage 2 bootloader
    mov si, msg_loading
    call print_string

    ; Reset disk system
    xor ax, ax
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    ; Load Stage 2 (sectors 2-10)
    mov ax, STAGE2_SEGMENT
    mov es, ax
    mov bx, STAGE2_OFFSET
    mov ah, 0x02            ; Read sectors
    mov al, 8               ; Number of sectors
    mov ch, 0               ; Cylinder 0
    mov cl, 2               ; Start from sector 2
    mov dh, 0               ; Head 0
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    ; Load kernel (sectors 10-74, 64 sectors = 32KB)
    mov ax, KERNEL_SEGMENT
    mov es, ax
    mov bx, KERNEL_OFFSET
    mov ah, 0x02
    mov al, 64              ; 64 sectors (32KB kernel)
    mov ch, 0
    mov cl, 10
    mov dh, 0
    mov dl, [boot_drive]
    int 0x13
    jc disk_error

    ; Print success
    mov si, msg_done
    call print_string

    ; Jump to Stage 2
    jmp STAGE2_SEGMENT:STAGE2_OFFSET

disk_error:
    mov si, msg_error
    call print_string
    jmp $

; Print string routine (SI = string pointer)
print_string:
    pusha
.loop:
    lodsb
    test al, al
    jz .done
    mov ah, 0x0E
    mov bx, 0x0007
    int 0x10
    jmp .loop
.done:
    popa
    ret

; Data
boot_drive: db 0
msg_boot:    db 'AtomOS Bootloader v1.0', 13, 10, 0
msg_loading: db 'Loading kernel...', 13, 10, 0
msg_done:    db 'Done! Starting Stage 2...', 13, 10, 0
msg_error:   db 'Disk Error!', 13, 10, 0

; Padding and boot signature
times 510 - ($ - $$) db 0
dw 0xAA55
