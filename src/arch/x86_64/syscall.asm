bits 64

; Code adapted from brutal
global _syscall_entry 
_syscall_entry:
    swapgs               ; swap from USER gs to KERNEL gs
    mov [gs:0x8], rsp    ; save current stack to the local cpu structure
    mov rsp, [gs:0x0]    ; use the kernel syscall stack

    push qword 0x3b      ; user data
    push qword [gs:0x8]  ; saved stack
    push r11             ; saved rflags
    push qword 0x43      ; user code
    push rcx             ; current IP

    push qword 0
    push qword 0

 
    cld
    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp

    extern syscall_entry 
    call syscall_entry

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    mov rsp, [gs:0x8]
    swapgs
    o64 sysret
