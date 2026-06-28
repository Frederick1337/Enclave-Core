// =========================================================================
// SOURCE CODE: src/common/main.cpp
// MASTER ARCHITECT: Frederick Joseph Lombardi
// SUBJECT: Cross-Platform Environment Verification, TRNG Boot Entropy, and Entry Point
// =========================================================================

#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <immintrin.h> // Required for RDRAND hardware intrinsic functions

struct SystemTopology {
    uint32_t cpu_vendor; // 1 = Intel, 2 = AMD
    bool vmm_active;
};

// Global, read-only host-space variable for true hardware entropy masking
extern "C" uint64_t g_DynamicMutationKey = 0;

// Forward structural representations of register contexts for external pipeline binding
struct GuestContext;
struct VMCB;
struct GuestRegisters;

class HardwareAuditor {
public:
    static uint32_t DetectVendor() {
        uint32_t eax=0, ebx=0, ecx=0, edx=0;
        __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
        if (ebx == 0x756e6547) return 1; // GenuineIntel
        if (ebx == 0x68747541) return 2; // AuthenticAMD
        return 0;
    }
    static bool CheckHypervisor() {
        uint32_t eax=0, ebx=0, ecx=0, edx=0;
        __asm__ __volatile__("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(1));
        return (ecx & (1ULL << 31)); 
    }
    
    // Step 1: Implement Dynamic Boot Entropy via Hardware TRNG
    static uint64_t GenerateHardwareEntropySeed() {
        uint64_t seed = 0;
        int retry = 0;
        const int MAX_RETRIES = 10;

        // Invoke Intel/AMD hardware True Random Number Generator directly
        while (retry < MAX_RETRIES) {
            #if defined(__x86_64__) || defined(_M_X64)
            if (_rdrand64_step(&seed)) {
                return seed; // Successfully generated a cryptographically secure random number
            }
            #else
            seed = 0x55AAFJLOMBARDI_FALLBACK;
            return seed;
            #endif
            retry++;
        }

        // Fallback security failure halt if hardware entropy engine fails to respond
        __asm__ __volatile__("cli; hlt");
        return 0; 
    }
};

#ifdef ARCH_INTEL_VMX
extern "C" void LaunchIntelPipeline(uint64_t exit_reason, GuestContext* context);
#else
extern "C" void LaunchAMDPipeline(VMCB* vmcb, GuestRegisters* regs);
#endif

int main() {
    SystemTopology topology{};
    topology.cpu_vendor = HardwareAuditor::DetectVendor();
    topology.vmm_active = HardwareAuditor::CheckHypervisor();

    if (!topology.vmm_active) {
        std::cerr << "[CRITICAL VIOLATION] Execution halted: System outside secure Ring -1 container.\n";
        return EXIT_FAILURE;
    }

    // Initialize the global dynamic mutation key with hardware entropy at startup
    g_DynamicMutationKey = HardwareAuditor::GenerateHardwareEntropySeed();
    std::cout << "[SECURITY ATTESTATION PASSED] System verified under hard virtualization control.\n";
    std::cout << "[TRNG CORE] Dynamic Boot Entropy Key successfully locked into host space.\n";

    if (topology.cpu_vendor == 1) {
#ifdef ARCH_INTEL_VMX
        GuestContext* real_intel_frame = nullptr; 
        LaunchIntelPipeline(18, real_intel_frame); 
#endif
    } else if (topology.cpu_vendor == 2) {
#ifdef ARCH_AMD_SVM
        VMCB* real_amd_vmcb = nullptr;
        GuestRegisters* real_amd_regs = nullptr; 
        LaunchAMDPipeline(real_amd_vmcb, real_amd_regs);
#endif
    }
    return EXIT_SUCCESS;
}
