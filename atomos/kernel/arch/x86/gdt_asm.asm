; AtomOS - GDT Assembly Helpers
; Load GDT and update segment registers

[BITS 32]

global gdt_flush
global tss_flush

; void gdt_flush(uint32_t gdt_ptr)
gdt_flush:
    mov eax, [esp + 4]      ; Get GDT pointer
    lgdt [eax]              ; Load GDT
    
    ; Update segment registers
    mov ax, 0x10            ; Kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    ; Far jump to update CS
    jmp 0x08:.flush         ; Kernel code segment
.flush:
    ret

; void tss_flush(void)
tss_flush:
    mov ax, 0x28            ; TSS segment (index 5 * 8 = 0x28)
    or ax, 0x03             ; Set RPL to 3
    ltr ax                  ; Load task register
    ret
