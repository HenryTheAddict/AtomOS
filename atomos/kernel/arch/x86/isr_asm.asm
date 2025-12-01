; AtomOS - Interrupt Service Routine Assembly Stubs
; Creates ISR and IRQ handler stubs that call C handlers

[BITS 32]

; External C handlers
extern isr_handler
extern irq_handler

; IDT flush function
global idt_flush
idt_flush:
    mov eax, [esp + 4]
    lidt [eax]
    ret

; Macro for ISR without error code
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push dword 0            ; Dummy error code
    push dword %1           ; Interrupt number
    jmp isr_common
%endmacro

; Macro for ISR with error code
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    push dword %1           ; Interrupt number
    jmp isr_common
%endmacro

; Macro for IRQ
%macro IRQ 2
global irq%1
irq%1:
    push dword 0            ; Dummy error code
    push dword %2           ; Interrupt number
    jmp irq_common
%endmacro

; Exception ISRs (0-31)
ISR_NOERRCODE 0     ; Division By Zero
ISR_NOERRCODE 1     ; Debug
ISR_NOERRCODE 2     ; NMI
ISR_NOERRCODE 3     ; Breakpoint
ISR_NOERRCODE 4     ; Overflow
ISR_NOERRCODE 5     ; Bound Range Exceeded
ISR_NOERRCODE 6     ; Invalid Opcode
ISR_NOERRCODE 7     ; Device Not Available
ISR_ERRCODE   8     ; Double Fault
ISR_NOERRCODE 9     ; Coprocessor Segment Overrun
ISR_ERRCODE   10    ; Invalid TSS
ISR_ERRCODE   11    ; Segment Not Present
ISR_ERRCODE   12    ; Stack-Segment Fault
ISR_ERRCODE   13    ; General Protection Fault
ISR_ERRCODE   14    ; Page Fault
ISR_NOERRCODE 15    ; Reserved
ISR_NOERRCODE 16    ; x87 Floating-Point
ISR_ERRCODE   17    ; Alignment Check
ISR_NOERRCODE 18    ; Machine Check
ISR_NOERRCODE 19    ; SIMD Floating-Point
ISR_NOERRCODE 20    ; Virtualization
ISR_ERRCODE   21    ; Control Protection
ISR_NOERRCODE 22    ; Reserved
ISR_NOERRCODE 23    ; Reserved
ISR_NOERRCODE 24    ; Reserved
ISR_NOERRCODE 25    ; Reserved
ISR_NOERRCODE 26    ; Reserved
ISR_NOERRCODE 27    ; Reserved
ISR_NOERRCODE 28    ; Hypervisor Injection
ISR_NOERRCODE 29    ; VMM Communication
ISR_ERRCODE   30    ; Security Exception
ISR_NOERRCODE 31    ; Reserved

; IRQs (mapped to 32-47)
IRQ 0, 32           ; Timer
IRQ 1, 33           ; Keyboard
IRQ 2, 34           ; Cascade
IRQ 3, 35           ; COM2
IRQ 4, 36           ; COM1
IRQ 5, 37           ; LPT2
IRQ 6, 38           ; Floppy
IRQ 7, 39           ; LPT1
IRQ 8, 40           ; CMOS RTC
IRQ 9, 41           ; Free
IRQ 10, 42          ; Free
IRQ 11, 43          ; Free
IRQ 12, 44          ; PS/2 Mouse
IRQ 13, 45          ; FPU
IRQ 14, 46          ; Primary ATA
IRQ 15, 47          ; Secondary ATA

; System call (INT 0x80)
ISR_NOERRCODE 128

; Common ISR handler stub
isr_common:
    pusha               ; Push all general-purpose registers
    
    push ds             ; Save segment registers
    push es
    push fs
    push gs
    
    mov ax, 0x10        ; Kernel data segment
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp            ; Push pointer to registers struct
    call isr_handler
    add esp, 4
    
    pop gs              ; Restore segment registers
    pop fs
    pop es
    pop ds
    
    popa                ; Restore general-purpose registers
    add esp, 8          ; Clean up error code and int number
    iret

; Common IRQ handler stub
irq_common:
    pusha
    
    push ds
    push es
    push fs
    push gs
    
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    push esp
    call irq_handler
    add esp, 4
    
    pop gs
    pop fs
    pop es
    pop ds
    
    popa
    add esp, 8
    iret
