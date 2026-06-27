; =========================================================================
; SYSTEM CONFIGURATION FILE: src/boot/bootloader.asm
; MASTER ARCHITECT: Frederick Joseph Lombardi
; SUBJECT: x86_64 Low-Level Assembly Virtualization Initialization
; =========================================================================

global initialize_vmx_operation
global execute_vmlaunch_sequence
global initialize_svm_operation

section .text

align 16
initialize_vmx_operation:
    mov rax, cr4
    bts rax, 5                      ; CR4.VMXE = 1 (Enable Intel Virtualization)
    mov cr4, rax

    mov ecx, 0x3A                   ; IA32_FEATURE_CONTROL MSR
    rdmsr
    test eax, 1                     ; Verify Lock bit
    jnz .locked
    or eax, 5                       ; Force Lock + Enable outside SMX
    wrmsr
.locked:
    vmxon [rdi]                     ; Enter VMX Root Operation
    jc .fail
    jnc .success
.fail:
    mov rax, 1
    ret
.success:
    mov rax, 0
    ret

align 16
execute_vmlaunch_sequence:
    vmptrld [rdi]                   ; Load active VMCS tracking block
    jc .fail_ptr
    
    push rbp
    push rbx
    push r12
    push r13
    push r14
    push r15

    vmlaunch                        ; Execute Guest OS demotion sweep
    
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbx
    pop rbp
    mov rax, 3                      ; Fallthrough error code
    ret
.fail_ptr:
    mov rax, 4
    ret

align 16
initialize_svm_operation:
    mov ecx, 0xC0000080             ; EFER MSR
    rdmsr
    bts eax, 12                     ; EFER.SVME = 1 (Enable AMD Virtualization)
    wrmsr
    
    mov rax, rdi                    ; Move VMCB pointer to RAX
    vmrun                           ; Enter AMD Guest Mode Execution
    mov rax, 0
    ret
