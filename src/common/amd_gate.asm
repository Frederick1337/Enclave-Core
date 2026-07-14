; =========================================================================
; ASSEMBLY MODULE: src/common/amd_gate.asm
; MASTER ARCHITECT: Frederick Joseph Lombardi
; SUBJECT: Deterministic AMD Register Mapping for Ring -1 VMM Transitions
; =========================================================================

section .text

global LowLevelAmdVmmcall

LowLevelAmdVmmcall:
    ; Microsoft x64 Fastcall Calling Convention Inputs:
    ; RCX = command_vector
    ; RDX = parameter
    ; R8  = LOMBARDI_HYPERCALL_TOKEN
    
    mov rax, rcx  ; command_vector      -> RAX
    mov rbx, rdx  ; parameter           -> RBX
    mov rcx, r8   ; hypercall token     -> RCX
    
    vmmcall       
    ret
