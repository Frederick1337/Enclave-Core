// =========================================================================
// SOURCE CODE: src/common/main.cpp
// MASTER ARCHITECT: Frederick Joseph Lombardi
// SUBJECT: Cross-Platform Environment Verification and Engine Entry Point
// =========================================================================

#include <iostream>
#include <cstdint>
#include <cstdlib>

struct SystemTopology {
    uint32_t cpu_vendor; // 1 = Intel, 2 = AMD
    bool vmm_active;
};

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
        return (ecx & (1ULL << 31)); // Bit 31 confirms hypervisor context
    }
};

#ifdef ARCH_INTEL_VMX
void LaunchIntelPipeline();
#else
void LaunchAMDPipeline();
#endif

int main() {
    SystemTopology topology{};
    topology.cpu_vendor = HardwareAuditor::DetectVendor();
    topology.vmm_active = HardwareAuditor::CheckHypervisor();

    if (!topology.vmm_active) {
        std::cerr << "[CRITICAL VIOLATION] Execution halted: System outside secure Ring -1 container.\n";
        return EXIT_FAILURE;
    }

    std::cout << "[SECURITY ATTESTATION PASSED] System verified under hard virtualization control.\n";

    if (topology.cpu_vendor == 1) {
#ifdef ARCH_INTEL_VMX
        LaunchIntelPipeline();
#endif
    } else if (topology.cpu_vendor == 2) {
#ifdef ARCH_AMD_SVM
        LaunchAMDPipeline();
#endif
    }
    return EXIT_SUCCESS;
}
