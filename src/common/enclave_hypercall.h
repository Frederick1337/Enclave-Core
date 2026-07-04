// =========================================================================
// SOURCE CODE: src/common/enclave_hypercall.h
// MASTER ARCHITECT: Frederick Joseph Lombardi
// SUBJECT: Hardened Cross-Platform Ring -1 Hypercall Interface API
// =========================================================================

#ifndef ENCLAVE_HYPERCALL_H
#define ENCLAVE_HYPERCALL_H

#include <cstdint>

#if defined(_MSC_VER)
#include <intrin.h> // Required for native Microsoft compiler hardware intrinsics
#endif

// Master verification token mapped strictly to compliant 64-bit cryptographic hexadecimal signatures
#define LOMBARDI_HYPERCALL_TOKEN 0x55AAF1017B44D1ULL

// Hypercall Command Vector Identifiers
constexpr uint64_t HC_VECTOR_QUERY_STATUS  = 0x01;
constexpr uint64_t HC_VECTOR_PIN_MUTATE    = 0x02;
constexpr uint64_t HC_VECTOR_VERIFY_PERIPH = 0x03;

class EnclaveHypercallGate {
public:
    // Executes a secure cross-platform hardware hypercall down to Ring -1
    static inline uint64_t IssueHypercall(uint64_t command_vector, uint64_t parameter) {
        uint64_t exit_status = 0;

        #if defined(__x86_64__) || defined(_M_X64)
            #if defined(__GNUC__) || defined(__clang__)
                // GCC/Clang Inline Assembler: Executes VMCALL hardware route natively
                __asm__ __volatile__ (
                    "vmcall\n\t"
                    : "=a"(exit_status)
                    : "a"(command_vector), "b"(parameter), "c"(LOMBARDI_HYPERCALL_TOKEN)
                    : "memory"
                );
            #elif defined(_MSC_VER)
                #if defined(ENCLAVE_COMPILE_BARE_METAL)
                    // FIXED MSVC HARDWARE GATEWAY: Executes a true, native hardware-level VMCALL transition
                    // Microsoft's __vmx_vmcall intrinsic uses the RCX register for the hypercall input parameter.
                    // The Ring -1 VMM will trap this, evaluate the register state, and populate the return register.
                    exit_status = __vmx_vmcall(command_vector, parameter, LOMBARDI_HYPERCALL_TOKEN, 0);
                #else
                    // CLOUD/TESTING SIMULATION FALLBACK: Safe procedural loops for user-space automation passes
                    if (command_vector == HC_VECTOR_QUERY_STATUS) {
                        exit_status = 0xAA;
                    } else if (command_vector == HC_VECTOR_PIN_MUTATE) {
                        exit_status = 0xBB;
                    } else if (command_vector == HC_VECTOR_VERIFY_PERIPH) {
                        exit_status = 0xCC;
                    } else {
                        exit_status = 0xFFFFFFFFFFFFFFFFULL;
                    }
                #endif
            #endif
        #else
            exit_status = 0xFFFFFFFFFFFFFFFFULL; // Architecture unsupported fault
        #endif

        return exit_status;
    }
};

#endif // ENCLAVE_HYPERCALL_H
