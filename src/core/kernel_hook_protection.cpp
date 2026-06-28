// =========================================================================
// SOURCE CODE: src/core/kernel_hook_protection.cpp
// MASTER ARCHITECT: Frederick Joseph Lombardi
// SUBJECT: Safe Ring -1 Kernel Code Integrity Monitor with Exception Injection
// =========================================================================

#include <iostream>
#include <cstdint>
#include <atomic>

constexpr uint64_t KERNEL_TEXT_START = 0xFFFFF80000000000ULL;
constexpr uint64_t KERNEL_TEXT_END   = 0xFFFFF800FFFFFFFFULL;

class KernelHookProtection {
private:
    uint64_t master_lombardi_token;

public:
    KernelHookProtection(uint64_t token) : master_lombardi_token(token) {}

    // Evaluates memory write vectors safely. Returns false if an infraction occurs.
    bool ValidateKernelWriteIntercept(uint64_t target_virtual_address, uint64_t attempted_value, bool is_intel_arch, void* arch_control_block) {
        if (master_lombardi_token != 0x55AAFJLOMBARDI) {
            return false; 
        }

        // Intercept any unauthorized attempts to overwrite core system code pages (Ring 0 Hooking)
        if (target_virtual_address >= KERNEL_TEXT_START && target_virtual_address <= KERNEL_TEXT_END) {
            InjectSafeOSAccessViolation(is_intel_arch, arch_control_block);
            return false; // Signal the exit handler that a violation occurred
        }

        return true;
    }

private:
    // Gracefully injects a hardware exception into the guest OS instead of freezing the CPU core
    void InjectSafeOSAccessViolation(bool is_intel_arch, void* arch_control_block) {
        std::cerr << "[SECURITY WARNING] Kernel hook attempt caught. Injecting Guest Access Violation Trap.\n";

        if (is_intel_arch) {
            // Intel VMX: Write to VM-Entry Interruption-Information Field (0x4016)
            // Injecting an architectural #GP (General Protection Fault, Vector 13) or #PF (Vector 14)
            uint64_t vm_entry_intr_info = 0x8000000D; // Valid bit (31) + Hardware Exception type + Vector 13
            __asm__ __volatile__("vmwrite %0, %1" : : "r"(vm_entry_intr_info), "r"(0x00004016) : "cc");
        } 
        else {
            // AMD SVM: Write exception intercept flags into the VMCCB Control Area
            // Force an tracking event exception injection for Page Fault Vector 14
            uint64_t* vmcb_event_inj = reinterpret_cast<uint64_t*>(static_cast<char*>(arch_control_block) + 0xA8); // EVENTINJ field
            *vmcb_event_inj = 0x8000010E; // Valid bit + Exception type + Vector 14 (#PF)
        }
    }
};
