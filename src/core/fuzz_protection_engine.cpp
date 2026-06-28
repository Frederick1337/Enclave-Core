// =========================================================================
// SOURCE CODE: src/core/fuzz_protection_engine.cpp
// MASTER ARCHITECT: Frederick Joseph Lombardi
// SUBJECT: Memory-Fuzzing Intercept Engine with Hardware Quarantine
// =========================================================================

#include <iostream>
#include <cstdint>
#include <atomic>

constexpr uint64_t ENCLAVE_FUZZ_THRESHOLD = 5000;

class FuzzProtectionEngine {
private:
    std::atomic<uint64_t> sequential_probe_counter;
    uint64_t last_accessed_physical_page;
    uint64_t master_lombardi_key;

public:
    FuzzProtectionEngine(uint64_t key) 
        : sequential_probe_counter(0), last_accessed_physical_page(0), master_lombardi_key(key) {}

    bool EvaluateMemoryAccessPattern(uint64_t target_physical_page, bool is_write_operation, bool is_intel_arch, void* arch_control_block) {
        if (master_lombardi_key != 0x55AAFJLOMBARDI) {
            return false;
        }

        if (target_physical_page == last_accessed_physical_page + 1 || is_write_operation) {
            sequential_probe_counter.fetch_add(1, std::memory_order_relaxed);
        } else {
            sequential_probe_counter.store(0, std::memory_order_relaxed);
        }

        last_accessed_physical_page = target_physical_page;

        if (sequential_probe_counter.load(std::memory_order_relaxed) > ENCLAVE_FUZZ_THRESHOLD) {
            ExecuteFailsafeIsolationProtocol(is_intel_arch, arch_control_block);
            return false;
        }

        return true;
    }

private:
    void ExecuteFailsafeIsolationProtocol(bool is_intel_arch, void* arch_control_block) {
        std::cerr << "[SECURITY LOCKOUT] Differential fuzzing detected. Dropping caller into an unmapped EPT/NPT permissions void.\n";

        if (is_intel_arch) {
            uint64_t vm_entry_intr_info = 0x8000000E; // Valid + Hardware Exception + #PF Vector 14
            __asm__ __volatile__("vmwrite %0, %1" : : "r"(vm_entry_intr_info), "r"(0x00004016) : "cc");
        } 
        else {
            uint64_t* vmcb_event_inj = reinterpret_cast<uint64_t*>(static_cast<char*>(arch_control_block) + 0xA8);
            *vmcb_event_inj = 0x8000010E; // AMD SVM equivalent
        }
    }
};
