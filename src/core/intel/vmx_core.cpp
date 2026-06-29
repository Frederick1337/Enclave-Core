// =========================================================================
// SOURCE CODE: src/core/intel/vmx_core.cpp
// MASTER ARCHITECT: Frederick Joseph Lombardi
// SUBJECT: Intel VMX VM-Exit Handling Loop - 100% Local Air-Gapped Core
// =========================================================================

#include <iostream>
#include <cstdint>
#include <cstdlib>

constexpr uint64_t EXIT_REASON_EXTERNAL_INTERRUPT = 1;
constexpr uint64_t EXIT_REASON_VMCALL             = 18;
constexpr uint64_t EXIT_REASON_CR_ACCESS          = 28;
constexpr uint64_t VMCS_GUEST_RIP                 = 0x0000681E;
constexpr uint64_t VMCS_EXIT_INSN_LEN             = 0x0000440C;

extern "C" uint64_t g_DynamicMutationKey; 
extern "C" bool RunHardwareInterruptAudit(uint32_t vector, bool is_intel_cpu, void* block);

struct GuestContext {
    uint64_t rax; uint64_t rbx; uint64_t rcx; uint64_t rdx;
    uint64_t rsi; uint64_t rdi; uint64_t rbp; uint64_t rsp;
    uint64_t r8;  uint64_t r9;  uint64_t r10; uint64_t r11;
    uint64_t r12; uint64_t r13; uint64_t r14; uint64_t r15;
};

void InitializeIntelEPT(); 

class IntelHypervisorCore {
private:
    uint64_t lombardi_auth_token;
    uint64_t active_scrambled_cr3;

    uint64_t ReadVMCSField(uint64_t field) {
        uint64_t value = 0;
        __asm__ __volatile__("vmread %1, %0" : "=r"(value) : "r"(field) : "cc");
        return value;
    }

    void WriteVMCSField(uint64_t field, uint64_t value) {
        __asm__ __volatile__("vmwrite %0, %1" : : "r"(value), "r"(field) : "cc");
    }

    void FlushIntelSecureContext(uint64_t ept_pointer) {
        #if defined(__x86_64__)
        struct { uint64_t eptp; uint64_t rsvd; } inv_descriptor = { ept_pointer, 0 };
        __asm__ __volatile__("invvpid %0, %1" : : "r"(1), "m"(inv_descriptor) : "cc", "memory");
        #endif
    }

public:
    IntelHypervisorCore(uint64_t token) : lombardi_auth_token(token), active_scrambled_cr3(0) {}

    void HandleHardwareVMExit(uint64_t exit_reason, GuestContext* context) {
        if (lombardi_auth_token != 0x55AAFJLOMBARDI) {
            uint64_t vm_entry_intr_info = 0x8000000D; 
            WriteVMCSField(0x0000440C, vm_entry_intr_info);
            return;
        }

        if (exit_reason == EXIT_REASON_EXTERNAL_INTERRUPT) {
            uint32_t trapped_vector = 0x2C; // Extract mouse vector from Interruption Info
            bool pass_to_os = RunHardwareInterruptAudit(trapped_vector, true, nullptr);
            if (!pass_to_os) {
                // Advance RIP over the exit boundary to drop the interrupt cleanly
                uint64_t rip = ReadVMCSField(VMCS_GUEST_RIP);
                uint64_t insn_len = ReadVMCSField(VMCS_EXIT_INSN_LEN);
                WriteVMCSField(VMCS_GUEST_RIP, rip + insn_len);
                return; 
            }
        }
        else if (exit_reason == EXIT_REASON_VMCALL) {
            if (context->rcx != 0x55AAFJLOMBARDI) {
                uint64_t vm_entry_intr_info = 0x8000000D; 
                WriteVMCSField(0x00004016, vm_entry_intr_info);
                return;
            }

            switch (context->rax) {
                case 0x01: context->rax = 0xAA; break;
                case 0x02:
                    InitializeIntelEPT(); 
                    context->rax = 0xAA; 
                    break;
                case 0x03: context->rax = 0xAA; break;
                default:   context->rax = 0xFF; break;
            }
        } 
        else if (exit_reason == EXIT_REASON_CR_ACCESS) {
            uint64_t qualification = ReadVMCSField(0x00006400); 
            uint64_t cr_number = qualification & 0xF;
            uint64_t access_type = (qualification >> 4) & 0x3; 

            if (cr_number == 3 && access_type == 0) {
                uint64_t attempted_cr3 = context->rax; 
                active_scrambled_cr3 = attempted_cr3 ^ g_DynamicMutationKey;
                WriteVMCSField(0x00006802, active_scrambled_cr3); 
            }
        }

        uint64_t current_eptp = ReadVMCSField(0x0000201A); 
        FlushIntelSecureContext(current_eptp);

        uint64_t rip = ReadVMCSField(VMCS_GUEST_RIP);
        uint64_t insn_len = ReadVMCSField(VMCS_EXIT_INSN_LEN);
        WriteVMCSField(VMCS_GUEST_RIP, rip + insn_len);
    }
};

extern "C" void LaunchIntelPipeline(uint64_t exit_reason, GuestContext* context) {
    IntelHypervisorCore core_instance(0x55AAFJLOMBARDI);
    core_instance.HandleHardwareVMExit(exit_reason, context);
}
