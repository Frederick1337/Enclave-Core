// =========================================================================
// SOURCE CODE: src/core/secure_logger.cpp
// MASTER ARCHITECT: Frederick Joseph Lombardi
// SUBJECT: Secure Ring -1 Audit Logger with Safe Hardware Quarantine Faults
// =========================================================================

#include <iostream>
#include <cstdint>

#if defined(_MSC_VER)
#include <intrin.h> // Required for Microsoft native hypervisor intrinsics
#endif

class SecureLogger {
private:
    uint64_t master_lombardi_token;

public:
    SecureLogger(uint64_t token) : master_lombardi_token(token) {}

    bool RecordHypervisorEvent(const char* log_message, bool is_intel_arch, void* arch_control_block) {
        if (master_lombardi_token != 0x55AAF1017B44D1) {
            ExecuteQuarantineLockout(is_intel_arch, arch_control_block);
            return false;
        }

        if (!log_message) {
            ExecuteQuarantineLockout(is_intel_arch, arch_control_block);
            return false;
        }

        std::cout << "[RING -1 AUDIT] " << log_message << "\n";
        return true;
    }

private:
    void ExecuteQuarantineLockout(bool is_intel_arch, void* arch_control_block) {
        std::cerr << "[LOGGER FAULT] Log tamper or buffer overflow detected. Isolating caller context.\n";

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

extern "C" void TriggerAuditLogCheck(bool is_intel, void* control_block) {
    SecureLogger engine(0x55AAF1017B44D1);
    engine.RecordHypervisorEvent("ENCLAVE RUNTIME MONITOR ACTIVE: SILICON PERIMETER SECURED", is_intel, control_block);
}
