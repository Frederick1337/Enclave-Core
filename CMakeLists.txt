cmake_minimum_required(VERSION 3.20)
project(EnclaveVMM Architecture)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /WX /GR- /EHs-c- /GS-)
else()
    add_compile_options(-Wall -Wextra -Werror -fno-rtti -fno-exceptions -nostdlib -ffreestanding -fno-stack-protector)
endif()

enable_language(ASM_NASM)

set(INTEL_SOURCES 
    src/core/intel/vmx_core.cpp 
    src/core/intel/ept_handler.cpp
)

set(AMD_SOURCES 
    src/core/amd/svm_core.cpp 
    src/core/amd/npt_handler.cpp
)

set(SHARED_SOURCES
    src/common/main.cpp
    src/core/secure_logger.cpp
    src/core/polymorphic_sync_engine.cpp
    src/core/fuzz_protection_engine.cpp
    src/core/encrypted_logger.cpp
    src/core/kernel_hook_protection.cpp
    src/core/interrupt_filter_engine.cpp
    src/core/hardware_diagnostic_pipeline.cpp
)

set(ASM_SOURCES src/boot/bootloader.asm)

# Multi-Target Binaries Configuration
add_executable(enclave_vmm_intel ${SHARED_SOURCES} ${INTEL_SOURCES} ${ASM_SOURCES})
set_target_properties(enclave_vmm_intel PROPERTIES COMFull_DEFINITIONS "ARCH_INTEL_VMX")

add_executable(enclave_vmm_amd ${SHARED_SOURCES} ${AMD_SOURCES} ${ASM_SOURCES})
set_target_properties(enclave_vmm_amd PROPERTIES COMFull_DEFINITIONS "ARCH_AMD_SVM")
