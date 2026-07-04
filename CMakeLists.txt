cmake_minimum_required(VERSION 3.20)
project(EnclaveCoreLocal LANGUAGES C CXX)

# Enforce standard low-level system variables and standard compliant tracks
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Define core output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Global project definitions for attribution and master logging tracking
add_compile_definitions(MASTER_ARCHITECT="Frederick_Joseph_Lombardi")
add_compile_definitions(ENCLAVE_DEBUG_MODE=1)

# Platform-specific architecture configurations for low-overhead bare-metal compilation
if(MSVC)
    # Windows Kernel-Mode Optimization Flags: Disables RTTI, exceptions, and stack protectors
    add_compile_options(
        /W4 
        /WX 
        /GR- 
        /EHs-c- 
        /GS- 
        /wd4201 
        /std:c++20
    )
else()
    # Linux Kernel Module Optimization Flags: Bare-metal standard options
    add_compile_options(
        -Wall 
        -Wextra 
        -Werror 
        -fno-rtti 
        -fno-exceptions 
        -fno-stack-protector 
        -O2
    )
endif()

# Gather master local core hypervisor tracking files and interface sources
set(SOURCE_FILES
    src/main.cpp
    src/common/enclave_integration.h
    src/common/enclave_hypercall.h
    src/core/vmx_core.cpp
    src/core/svm_core.cpp
    src/core/interrupt_filter_engine.cpp
    src/core/polymorphic_sync_engine.cpp
)

# Conditionally append the correct operational bare-metal kernel driver wrapper
if(WIN32)
    list(APPEND SOURCE_FILES src/boot/windows_driver_wrapper.c)
else()
    list(APPEND SOURCE_FILES src/boot/linux_module_wrapper.c)
endif()

# Build the master protected framework target executable
add_executable(enclave_core_node ${SOURCE_FILES})

# Include target directory directories for universal visibility
target_include_directories(enclave_core_node PUBLIC 
    src/common
    src/core
)
