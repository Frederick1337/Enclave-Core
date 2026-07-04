; =========================================================================
; ASSEMBLY MODULE: src/common/amd_gate.asm
; MASTER ARCHITECT: Frederick Joseph Lombardi
; SUBJECT: Deterministic AMD Register Mapping for Ring -1 VMM Transitions
; =========================================================================

.code

; Export symbol using standard C-linkage visibility
public LowLevelAmdVmmcall

LowLevelAmdVmmcall proc
    ; Microsoft x64 Fastcall Calling Convention Inputs:
    ; RCX = command_vector
    ; RDX = parameter
    ; R8  = LOMBARDI_HYPERCALL_TOKEN
    
    ; Enforce explicit hardware register constraints before the trap triggers:
    mov rax, rcx  ; command_vector      -> RAX (Expected by Ring -1 Core)
    mov rbx, rdx  ; parameter           -> RBX (Expected by Ring -1 Core)
    mov rcx, r8   ; hypercall token     -> RCX (Expected by Ring -1 Core)
    
    ; Execute true hardware privilege transition trap into AMD SVM operation
    vmmcall       
    
    ; Return to C++ compiler layer. 
    ; The exit status/confirmation code populated in RAX by the VMM passes back natively.
    ret           
LowLevelAmdVmmcall endproc

end
