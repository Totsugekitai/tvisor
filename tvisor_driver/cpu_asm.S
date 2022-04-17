.intel_syntax noprefix
.text

# int is_vmx_supported(void)
# return value:
# - true: 1
# - false: 0
# .global is_vmx_supported
#is_vmx_supported:
#    xor rax, rax
#    inc rax
#    cpuid
#    bt ecx, 0x5
#    jc vmx_supported
#vmx_not_supported:
#    mov rax, 0x0
#    jmp end
#vmx_supported:
#    mov rax, 0x1
#end:
#    ret

# void asm_get_cpuid(uint32_t level, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx)
.global asm_get_cpuid
asm_get_cpuid:
    mov rax, rdi
    push rsi
    push rdx
    push rcx
    push r8
    cpuid
    pop r8
    mov [r8], edx
    pop r8
    mov [r8], ecx
    pop r8
    mov [r8], ebx
    pop r8
    mov [r8], eax
    ret

# void asm_read_msr(uint32_t msr, uint32_t *low, uint32_t *high)
.global asm_read_msr
asm_read_msr:
    mov ecx, edi
    push rsi
    push rdx
    rdmsr
    pop rcx
    mov [rcx], edx
    pop rcx
    mov [rcx], eax
    ret

# void asm_write_msr(uint32_t msr, uint32_t low, uint32_t high)
.global asm_write_msr
asm_write_msr:
    mov ecx, edi
    mov eax, esi
    wrmsr
    ret

# uint64_t is_vmx_enabled(void)
.global is_vmx_enabled
is_vmx_enabled:
    xor rax, rax
    mov rax, cr4
    and rax, 0x2000
    ret

# void asm_enable_vmx(void)
.global asm_enable_vmx
asm_enable_vmx:
    xor rax, rax
    mov rax, cr4
    or  rax, 0x2000
    mov cr4, rax
    ret

# void asm_disable_vmx(void)
.global asm_disable_vmx
asm_disable_vmx:
    xor rax, rax
    mov rax, cr4
    and rax, 0xffffffffffffdfff
    mov cr4, rax
    ret