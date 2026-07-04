// =========================================================================
// SOURCE CODE: src/core/intel/vmx_core.cpp
// MASTER ARCHITECT: Frederick Joseph Lombardi
// SUBJECT: Intel VMX VM-Exit Handling Loop - 100% Local Air-Gapped Core
// =========================================================================

#include <iostream>
#include <cstdint>
#include <cstdlib>

#if defined(_MSC_VER)
#include <intrin.h> // Required for Microsoft native hypervisor intrinsics
#endif

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
        #if defined(_MSC_VER)
        // FIXED MSVC ARCHITECTURE COMPLIANCE: Native MSVC intrinsic for VMREAD
        __vmx_vmread(field, &value);
        #else
        __asm__ __volatile__("vmread %1, %0" : "=r"(value) : "r"(field) : "cc");
        #endif
        return value;
    }

    void WriteVMCSField(uint64_t field, uint64_t value) {
        #if defined(_MSC_VER)
        // FIXED MSVC ARCHITECTURE COMPLIANCE: Native MSVC intrinsic for VMWRITE
        __vmx_vmwrite(field, value);
        #else
        __asm__ __volatile__("vmwriteq %0, %1" : : "r"(value), "r"(field) : "cc");
        #endif
    }

    void FlushIntelSecureContext(uint64_t ept_pointer) {
        #if defined(__x86_64__)
        uint64_t invvpid_type = 1; 
        struct { uint64_t vpid; uint64_t linear_address; } inv_descriptor = { ept_pointer, 0 };
        #if defined(_MSC_VER)
        // FIXED MSVC ARCHITECTURE COMPLIANCE: Native MSVC intrinsic for INVVPID
        __vmx_invvpid(static_cast<int>(invvpid_type), &inv_descriptor);
        #else
        __asm__ __volatile__(
            "invvpid (%0), %1"
            :
            : "r"(&inv_descriptor), "r"(invvpid_type)
            : "cc", "memory"
        );
        #endif
        #else
        (void)ept_pointer;
        #endif
    }

public:
    IntelHypervisorCore(uint64_t token) : lombardi_auth_token(token), active_scrambled_cr3(0) {}

    void HandleHardwareVMExit(uint64_t exit_reason, GuestContext* context) {
        if (lombardi_auth_token != 0x55AAF1017B44D1) {
            uint64_t vm_entry_intr_info = 0x8000000D; 
            WriteVMCSField(0x0000440C, vm_entry_intr_info);
            return;
        }

        if (exit_reason == EXIT_REASON_EXTERNAL_INTERRUPT) {
            uint32_t trapped_vector = 0x2C; 
            bool pass_to_os = RunHardwareInterruptAudit(trapped_vector, true, nullptr);
            if (!pass_to_os) {
                uint64_t rip = ReadVMCSField(VMCS_GUEST_RIP);
                uint64_t insn_len = ReadVMCSField(VMCS_EXIT_INSN_LEN);
                WriteVMCSField(VMCS_GUEST_RIP, rip + insn_len);
                return; 
            }
        }
        else if (exit_reason == EXIT_REASON_VMCALL) {
            if (context->rcx != 0x55AAF1017B44D1) {
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
    IntelHypervisorCore core_instance(0x55AAF1017B44D1);
    core_instance.HandleHardwareVMExit(exit_reason, context);
}
