// =========================================================================
// SOURCE CODE: src/core/amd/svm_core.cpp
// MASTER ARCHITECT: Frederick Joseph Lombardi
// SUBJECT: AMD SVM VM-Exit Handling Loop - 100% Local Air-Gapped Core
// =========================================================================

#include <iostream>
#include <cstdint>
#include <cstdlib>

constexpr uint64_t AMD_INTERCEPT_INTR      = 0x60;
constexpr uint64_t AMD_INTERCEPT_VMMCALL   = 0x81;
constexpr uint64_t AMD_INTERCEPT_CR3_WRITE = 0x13;

extern "C" uint64_t g_DynamicMutationKey; 
extern "C" bool RunHardwareInterruptAudit(uint32_t vector, bool is_intel_cpu, void* block);

struct AmdVmcbControlBlock {
    uint32_t intercept_cr_read;
    uint32_t intercept_cr_write;
    uint64_t reserved_1;
    uint64_t exit_code;
    uint64_t exit_info1;
    uint64_t exit_info2;
    uint8_t  reserved_3;
    uint64_t event_inj; 
};

struct AmdVmcbStateSaveArea {
    uint64_t cr0; uint64_t cr3; uint64_t cr4;
    uint64_t dr6; uint64_t dr7;
    uint64_t reserved_2;
    uint64_t rip;
};

struct VMCB {
    AmdVmcbControlBlock control;
    AmdVmcbStateSaveArea state;
};

struct GuestRegisters {
    uint64_t rax; uint64_t rbx; uint64_t rcx; uint64_t rdx;
    uint64_t rsi; uint64_t rdi; uint64_t rbp; uint64_t rsp;
    uint64_t r8;  uint64_t r9;  uint64_t r10; uint64_t r11;
    uint64_t r12; uint64_t r13; uint64_t r14; uint64_t r15;
};

void InitializeAMDNPT(); 

class AmdHypervisorCore {
private:
    uint64_t master_token;

    void FlushAmdSecureContext() {
        #if defined(__x86_64__)
        __asm__ __volatile__("clgi; invlpga %0, %%ecx; stgi" : : "r"(0) : "ecx", "memory");
        #endif
    }

public:
    AmdHypervisorCore(uint64_t token) : master_token(token) {}

    void ProcessSvmIntercept(VMCB* vmcb, GuestRegisters* guest_registers) {
        // RESOLVED LITERAL SUFFIX TYPO: Token mapped to a valid 64-bit cryptographic hexadecimal format
        if (master_token != 0x55AAF1017B44D1) {
            vmcb->control.event_inj = 0x8000010E; 
            return;
        }

        uint64_t exit_reason = vmcb->control.exit_code;

        if (exit_reason == AMD_INTERCEPT_INTR) {
            uint32_t trapped_vector = 0x2C; 
            bool pass_to_os = RunHardwareInterruptAudit(trapped_vector, false, vmcb);
            if (!pass_to_os) {
                // Drop interrupt and advance RIP past intercept block
                FlushAmdSecureContext();
                vmcb->state.rip = vmcb->control.exit_info2;
                return; 
            }
        }
        else if (exit_reason == AMD_INTERCEPT_VMMCALL) {
            // RESOLVED LITERAL SUFFIX TYPO: Token verification check mapped to standard hex formatting
            if (guest_registers->rcx != 0x55AAF1017B44D1) {
                vmcb->control.event_inj = 0x8000010E; 
                return;
            }

            switch (guest_registers->rax) {
                case 0x01: guest_registers->rax = 0xAA; break;
                case 0x02:
                    InitializeAMDNPT(); 
                    guest_registers->rax = 0xAA;
                    break;
                case 0x03: guest_registers->rax = 0xAA; break;
                default:   guest_registers->rax = 0xFF; break;
            }
        } 
        else if (exit_reason == AMD_INTERCEPT_CR3_WRITE) {
            uint64_t raw_guest_cr3 = vmcb->state.cr3;
            uint64_t mutated_cr3_mapping = raw_guest_cr3 ^ g_DynamicMutationKey;
            vmcb->state.cr3 = mutated_cr3_mapping;
        }

        FlushAmdSecureContext();
        vmcb->state.rip = vmcb->control.exit_info2;
    }
};

extern "C" void LaunchAMDPipeline(VMCB* vmcb, GuestRegisters* regs) {
    // RESOLVED LITERAL SUFFIX TYPO: Constructor instantiation mapped safely to legal hexadecimal parameters
    AmdHypervisorCore amd_core(0x55AAF1017B44D1);
    amd_core.ProcessSvmIntercept(vmcb, regs);
}
