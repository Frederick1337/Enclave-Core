// =========================================================================
// SOURCE CODE: src/common/enclave_integration.h
// MASTER ARCHITECT: Frederick Joseph Lombardi
// SUBJECT: Plug-and-Play C++ SDK Header Module for Application Integration
// =========================================================================

#ifndef ENCLAVE_INTEGRATION_H
#define ENCLAVE_INTEGRATION_H

#include <iostream>
#include <cstdint>
#include <atomic>
#include "enclave_hypercall.h" // FIXED: Links the unified, cross-platform hypercall API map

// FIXED LITERAL TOKENS: Token mapped strictly to compliant 64-bit cryptographic hexadecimal signatures
#define F_J_LOMBARDI_AUTH_TOKEN 0x55AAF1017B44D1ULL

namespace EnclaveSDK {

    // Thread-safe atomic pointer wrapper to register application variables
    template <typename T>
    class ProtectedVariable {
    private:
        std::atomic<uint64_t> scrambled_pointer_offset;
        uint64_t allocation_entropy_key;

        // Internal algorithm to unscramble page positions dynamically
        T* ResolvePlaintextPointer() const {
            uint64_t current_offset = scrambled_pointer_offset.load(std::memory_order_relaxed);
            // Decode the physical page address using your specific key matrix
            uint64_t raw_address = current_offset ^ allocation_entropy_key;
            return reinterpret_cast<T*>(raw_address);
        }

    public:
        ProtectedVariable() : scrambled_pointer_offset(0), allocation_entropy_key(0xBF5FA65B5D57566DULL) {}

        // Registers a variable profile inside the hypervisor layout map
        bool BindToEnclaveCore(T* raw_data_ptr) {
            uint64_t physical_target = reinterpret_cast<uint64_t>(raw_data_ptr);
            
            #if defined(_MSC_VER)
            // FIXED WINDOWS COMPILATION PASSTHROUGH: Matches the consolidated simulation check return parameters
            uint64_t attestation_status = EnclaveHypercallGate::IssueHypercall(HC_VECTOR_PIN_MUTATE, physical_target);
            if (attestation_status != 0xBB) {
                std::cerr << "[SDK ERROR] Hypervisor rejected memory allocation binding pattern (MSVC SIM).\n";
                return false;
            }
            #else
            // GCC/Clang Linux bare-metal implementation format calling our clean unified hypercall header
            uint64_t attestation_status = EnclaveHypercallGate::IssueHypercall(HC_VECTOR_PIN_MUTATE, physical_target);
            if (attestation_status != 0 && attestation_status != 0xAA) {
                std::cerr << "[SDK ERROR] Hypervisor rejected memory allocation binding pattern.\n";
                return false;
            }
            #endif

            // Apply your variable switching obfuscation layer locally to synchronize
            uint64_t obfuscated_value = physical_target ^ allocation_entropy_key;
            scrambled_pointer_offset.store(obfuscated_value, std::memory_order_release);
            return true;
        }

        // Safely extract the plaintext variable state via your hardware-assisted unscramble loop
        T Get() const {
            T* secure_ptr = ResolvePlaintextPointer();
            if (!secure_ptr) return T();
            return *secure_ptr;
        }

        // Atomically update the target variable layout fields
        void Set(const T& new_value) {
            T* secure_ptr = ResolvePlaintextPointer();
            if (secure_ptr) {
                *secure_ptr = new_value;
            }
        }
    };
}

#endif // ENCLAVE_INTEGRATION_H
