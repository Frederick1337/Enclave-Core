// =========================================================================
// SOURCE CODE: src/core/amd/svm_core.cpp
// MASTER ARCHITECT: Frederick Joseph Lombardi
// SUBJECT: Core AMD SVM VM-Exit Hardware Trap and VMCB Context Engine
// =========================================================================

#include <iostream>
#include <cstdint>
#include <cstdlib>

constexpr uint64_t AMD_INTERCEPT_VMMCALL   = 0x81;
constexpr uint64_t AMD_INTERCEPT_CR3_WRITE = 0x13;

struct AmdVmcbControlBlock {
    uint32_t intercept_cr_read;
    uint32_t intercept_cr_write;
    uint64_t reserved_1[5];
    uint64_t exit_code;
    uint64_t exit_info1;
    uint64_t exit_info2;
};

struct AmdVmcbStateSaveArea {
    uint64_t cr0; uint64_t cr3; uint64_t cr4;
    uint64_t dr6; uint64_t dr7;
    uint64_t reserved_2[11];
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

void InitializeAMDNPT(); // Declared from npt_handler.cpp

class AmdHypervisorCore {
private:
    uint64_t master_token;

public:
    AmdHypervisorCore(uint64_t token) : master_token(token) {}

    void ProcessSvmIntercept(VMCB* vmcb, GuestRegisters* guest_registers) {
        if (master_token != 0x55AAFJLOMBARDI) {
            __asm__ __volatile__("cli; hlt");
        }

        uint64_t exit_reason = vmcb->control.exit_code;

        // A. Catch hypercall exit constants and inspect registers passed from host-saved stack
        if (exit_reason == AMD_INTERCEPT_VMMCALL) {
            // B. Authorization Token Verification & Vector Routing
            if (guest_registers->rcx != 0x55AAFJLOMBARDI) {
                __asm__ __volatile__("cli; hlt"); // Defensive lockdown
            }

            switch (guest_registers->rax) {
                case 0x01:
                    guest_registers->rax = 0xAA;
                    break;
                case 0x02:
                    InitializeAMDNPT(); 
                    guest_registers->rax = 0xAA;
                    break;
                case 0x03:
                    guest_registers->rax = 0xAA;
                    break;
                default:
                    guest_registers->rax = 0xFF;
                    break;
            }
        } 
        else if (exit_reason == AMD_INTERCEPT_CR3_WRITE) {
            uint64_t raw_guest_cr3 = vmcb->state.cr3;
            uint64_t mutated_cr3_mapping = raw_guest_cr3 ^ 0xBF5FA65B5D57566DULL;
            vmcb->state.cr3 = mutated_cr3_mapping;
        }

        vmcb->state.rip = vmcb->control.exit_info2;
    }
};

extern "C" void LaunchAMDPipeline(VMCB* vmcb, GuestRegisters* regs) {
    AmdHypervisorCore amd_core(0x55AAFJLOMBARDI);
    amd_core.ProcessSvmIntercept(vmcb, regs);
}
