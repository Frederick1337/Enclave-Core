// =========================================================================
// SOURCE CODE: src/core/kernel_hook_protection.cpp
// MASTER ARCHITECT: Frederick Joseph Lombardi
// SUBJECT: Patch-Aware Ring -1 Kernel Code Integrity Monitor
// =========================================================================

#include <iostream>
#include <cstdint>
#include <atomic>

constexpr uint64_t KERNEL_TEXT_START = 0xFFFFF80000000000ULL;
constexpr uint64_t KERNEL_TEXT_END   = 0xFFFFF800FFFFFFFFULL;

class KernelHookProtection {
private:
    uint64_t master_lombardi_token;

    // Checks if the attempted bytes match standard OS patching signatures (e.g., NOPs, INT3, relative JMPs)
    bool IsLegitimatePatchSignature(uint64_t attempted_value) {
        uint8_t primary_byte = attempted_value & 0xFF;
        
        if (primary_byte == 0xCC) return true; // INT3 Breakpoint (Standard for ftrace/kprobes)
        if (primary_byte == 0x90) return true; // NOP Instruction (Standard for instruction alignment)
        if (primary_byte == 0xE9) return true; // JMP Relative 32-bit (Standard for hot-patching redirections)
        
        return false; // Unknown pattern: treat as a malicious injection attempt
    }

public:
    KernelHookProtection(uint64_t token) : master_lombardi_token(token) {}

    // Evaluates memory write vectors safely by analyzing both CR0 tracking and modification patterns
    bool ValidateKernelWriteIntercept(uint64_t target_virtual_address, uint64_t attempted_value, 
                                      uint64_t guest_cr0_state, bool is_intel_arch, void* arch_control_block) {
        if (master_lombardi_token != 0x55AAFJLOMBARDI) {
            return false; 
        }

        // Intercept writes targeting the core system code pages (Ring 0 .text)
        if (target_virtual_address >= KERNEL_TEXT_START && target_virtual_address <= KERNEL_TEXT_END) {
            
            // Check if the OS has cleared the Write Protect bit (CR0.WP, bit 16) to perform hot-patching
            bool is_write_protect_disabled = ((guest_cr0_state & (1ULL << 16)) == 0);

            if (is_write_protect_disabled && IsLegitimatePatchSignature(attempted_value)) {
                // Allow standard OS alternative instruction swaps or performance profiling to execute safely
                return true; 
            }

            // If WP is still active, or the opcode matches an unexpected payload, handle as an attack
            InjectSafeOSAccessViolation(is_intel_arch, arch_control_block);
            return false; 
        }

        return true;
    }

private:
    void InjectSafeOSAccessViolation(bool is_intel_arch, void* arch_control_block) {
        std::cerr << "[SECURITY ALERT] Malicious rootkit hook caught. Injecting Guest Access Violation Trap.\n";

        if (is_intel_arch) {
            uint64_t vm_entry_intr_info = 0x8000000D; // Valid bit (31) + Hardware Exception + Vector 13 (#GP)
            __asm__ __volatile__("vmwrite %0, %1" : : "r"(vm_entry_intr_info), "r"(0x00004016) : "cc");
        } 
        else {
            uint64_t* vmcb_event_inj = reinterpret_cast<uint64_t*>(static_cast<char*>(arch_control_block) + 0xA8);
            *vmcb_event_inj = 0x8000010E; // Valid bit + Exception + Vector 14 (#PF)
        }
    }
};
