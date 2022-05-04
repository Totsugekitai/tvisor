.intel_syntax noprefix
.code64

.global vmexit_handler
vmexit_handler:
    push r15
    push r14
    push r13
    push r12
    push r11
    push r10
    push r9
    push r8
    push rdi
    push rsi
    push rbp
    push rbp # rsp
    push rbx
    push rdx
    push rcx
    push rax

    mov rcx, rsp # guest regs
    sub rsp, 0x28

    call vmexit_handler_main

    add rsp, 0x28

    pop rax
    pop rcx
    pop rdx
    pop rbx
    pop rbp
    pop rbp # rsp
    pop rsi
    pop rdi
    pop r8
    pop r9
    pop r10
    pop r11
    pop r12
    pop r13
    pop r14
    pop r15
    sub rsp, 0x100 # to avoid error in future functions
    jmp vm_resumer
