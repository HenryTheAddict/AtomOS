; AtomOS - Context Switch
; Low-level context switching between threads

[BITS 32]

global switch_context
global enter_usermode

; void switch_context(context_t *old, context_t *new)
; Saves current context and restores new context
switch_context:
    ; Get parameters
    mov eax, [esp + 4]      ; old context
    mov edx, [esp + 8]      ; new context
    
    ; Save old context if not NULL
    test eax, eax
    jz .restore
    
    ; Save general purpose registers
    mov [eax + 0], edi
    mov [eax + 4], esi
    mov [eax + 8], ebp
    mov [eax + 12], esp
    mov [eax + 16], ebx
    mov [eax + 20], edx
    mov [eax + 24], ecx
    
    ; Save return address as EIP
    mov ecx, [esp]
    mov [eax + 32], ecx
    
    ; Save flags
    pushfd
    pop ecx
    mov [eax + 40], ecx

.restore:
    ; Restore new context
    mov edi, [edx + 0]
    mov esi, [edx + 4]
    mov ebp, [edx + 8]
    mov esp, [edx + 12]
    mov ebx, [edx + 16]
    mov ecx, [edx + 24]
    mov eax, [edx + 28]
    
    ; Restore flags
    push dword [edx + 40]
    popfd
    
    ; Push return address
    push dword [edx + 32]
    
    ; Restore remaining registers
    mov edx, [edx + 20]
    
    ret

; void enter_usermode(uint32_t entry, uint32_t stack)
; Switch to ring 3 and jump to user code
enter_usermode:
    mov ecx, [esp + 4]      ; entry point
    mov edx, [esp + 8]      ; stack pointer
    
    ; Setup segment registers for ring 3
    mov ax, 0x23            ; User data segment | RPL 3
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Build interrupt return frame
    push 0x23               ; SS (user data)
    push edx                ; ESP
    push 0x202              ; EFLAGS (IF set)
    push 0x1B               ; CS (user code | RPL 3)
    push ecx                ; EIP
    
    iret
