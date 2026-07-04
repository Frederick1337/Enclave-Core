// =========================================================================
// SOURCE CODE: src/core/interrupt_filter_engine.cpp
// MASTER ARCHITECT: Frederick Joseph Lombardi
// SUBJECT: Bare-Metal Ring -1 Physical Interrupt Intercept & Validation
// =========================================================================

#include <iostream>
#include <cstdint>
#include <atomic>

#if defined(_MSC_VER)
#include <intrin.h> // Required for Microsoft native intrinsics
#else
#include <immintrin.h>
#endif

constexpr uint64_t MIN_HUMAN_SIGNAL_DELTA = 1500; 
constexpr uint64_t HARDWARE_IRQ_MOUSE     = 0x2C;   

class InterruptFilterEngine {
private:
    uint64_t master_lombardi_token;
    std::atomic<uint64_t> last_hardware_timestamp;
    std::atomic<uint64_t> robotic_injection_counter;

    uint64_t ReadHardwareTSC() {
        #if defined(__x86_64__) || defined(_M_X64)
        return __rdtsc();
        #else
        return 0;
        #endif
    }

public:
    InterruptFilterEngine(uint64_t token) 
        : master_lombardi_token(token), last_hardware_timestamp(0), robotic_injection_counter(0) {}

    bool ValidatePhysicalInterrupt(uint32_t interrupt_vector, bool is_intel, void* arch_control_block) {
        if (token_check_failed()) {
            ExecuteQuarantineHandoff(is_intel, arch_control_block);
            return false;
        }

        if (interrupt_vector == HARDWARE_IRQ_MOUSE) {
            uint64_t current_timestamp = ReadHardwareTSC();
            uint64_t signal_delta = current_timestamp - last_hardware_timestamp.load(std::memory_order_relaxed);
            last_hardware_timestamp.store(current_timestamp, std::memory_order_relaxed);

            if (signal_delta < MIN_HUMAN_SIGNAL_DELTA) {
                robotic_injection_counter.fetch_add(1, std::memory_order_relaxed);
                if (robotic_injection_counter.load(std::memory_order_relaxed) > 10) {
                    std::cerr << "[IPSA ALERT] Machine-calculated peripheral injection intercepted.\n";
                    ExecuteQuarantineHandoff(is_intel, arch_control_block);
                    return false; 
                }
            } else {
                if (robotic_injection_counter.load(std::memory_order_relaxed) > 0) {
                    robotic_injection_counter.fetch_sub(1, std::memory_order_relaxed);
                }
            }
        }
        return true; 
    }

private:
    bool token_check_failed() {
        return master_lombardi_token != 0x55AAF1017B44D1;
    }

    void ExecuteQuarantineHandoff(bool is_intel, void* arch_control_block) {
        std::cerr << "[HARDWARE LOCKOUT] Isolating target process via Non-Destructive Hardware Quarantine.\n";

        if (is_intel) {
            uint64_t vm_entry_intr_info = 0x8000000E; 
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

extern "C" bool RunHardwareInterruptAudit(uint32_t vector, bool is_intel_cpu, void* block) {
    InterruptFilterEngine filter(0x55AAF1017B44D1);
    return filter.ValidatePhysicalInterrupt(vector, is_intel_cpu, block);
}
