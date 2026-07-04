// =========================================================================
// SOURCE CODE: src/core/kernel_hook_protection.cpp
// MASTER ARCHITECT: Frederick Joseph Lombardi
// SUBJECT: Secure Context-Validated Kernel Integrity Monitor with Hardware Quarantine
// =========================================================================

#include <iostream>
#include <cstdint>
#include <atomic>

#if defined(_MSC_VER)
#include <intrin.h> // Required for Microsoft native hypervisor intrinsics
#endif

constexpr uint64_t KERNEL_TEXT_START = 0xFFFFF80000000000ULL;
constexpr uint64_t KERNEL_TEXT_END   = 0xFFFFF800FFFFFFFFULL;

class KernelHookProtection {
private:
    uint64_t master_lombardi_token;
    uint64_t trusted_system_cr3_base; 
    std::atomic<bool> is_protection_active; 

public:
    KernelHookProtection(uint64_t token, uint64_t system_cr3) 
        : master_lombardi_token(token), trusted_system_cr3_base(system_cr3), is_protection_active(false) {}

    void EngageDefensiveShield() {
        is_protection_active.store(true, std::memory_order_release);
        std::cout << "[SHIELD ACTIVE] Context-validated kernel monitoring engaged.\n";
    }

    bool ValidateKernelWriteIntercept(uint64_t target_virtual_address, uint64_t guest_active_cr3,
                                      uint64_t guest_cr0_state, bool is_intel_arch, void* arch_control_block) {
        if (master_lombardi_token != 0x55AAF1017B44D1) {
            return false; 
        }

        if (!is_protection_active.load(std::memory_order_acquire)) {
            return true;
        }

        if (target_virtual_address >= KERNEL_TEXT_START && target_virtual_address <= KERNEL_TEXT_END) {
            bool is_write_protect_disabled = ((guest_cr0_state & (1ULL << 16)) == 0);

            if (is_write_protect_disabled && (guest_active_cr3 == trusted_system_cr3_base)) {
                return true; 
            }

            ExecuteFailsafeIsolationProtocol(is_intel_arch, arch_control_block);
            return false; 
        }

        return true;
    }

private:
    void ExecuteFailsafeIsolationProtocol(bool is_intel_arch, void* arch_control_block) {
        std::cerr << "[SECURITY LOCKOUT] Host breach attempted. Isolating caller execution context.\n";

        if (is_intel_arch) {
            uint64_t vm_entry_intr_info = 0x8000000E; // Valid + Hardware Exception + #PF
            uint64_t vmcs_field_index = 0x00004016;
            #if defined(_MSC_VER)
            // FIXED MSVC ARCHITECTURE COMPLIANCE: Native MSVC intrinsic for VMWRITE
            __vmx_vmwrite(vmcs_field_index, vm_entry_intr_info);
            #else
            __asm__ __volatile__("vmwriteq %0, %1" : : "r"(vm_entry_intr_info), "r"(vmcs_field_index) : "cc");
            #endif
        } 
        else {
            uint64_t* vmcb_event_inj = reinterpret_cast<uint64_t*>(static_cast<char*>(arch_control_block) + 0xA8);
            *vmcb_event_inj = 0x8000010E; 
        }
    }
};
