cmake_minimum_required(VERSION 3.20)

# Explicitly enable C, CXX, and ASM_NASM languages for cross-compilation support
project(EnclaveCoreNetworkTemp LANGUAGES C CXX ASM_NASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Automatically gather all repository source files and templates
file(GLOB_RECURSE SOURCE_FILES 
    "src/*.cpp" 
    "src/*.c"
    "src/common/*.asm"
)

# =========================================================================
# OPERATING SYSTEM SOURCE FILE FILTER MATRIX
# =========================================================================

# Step 1: Always filter out incompatible Linux module files when building on Windows
if(WIN32)
    foreach(FILE_PATH ${SOURCE_FILES})
        if(FILE_PATH MATCHES "linux_module_wrapper\\.c$")
            list(REMOVE_ITEM SOURCE_FILES ${FILE_PATH})
        endif()
    endforeach()
endif()

# Step 2: Conditionally branch source compilation based on target environment parameters
if(DEFINED ENCLAVE_COMPILE_BARE_METAL)
    # PRODUCTION MODE: Exclude user-space application main loop to allow driver configuration
    foreach(FILE_PATH ${SOURCE_FILES})
        if(FILE_PATH MATCHES "main\\.cpp$")
            list(REMOVE_ITEM SOURCE_FILES ${FILE_PATH})
        endif()
    endforeach()
    
    message(STATUS "[TARGET CONFIGURATION] Preparing production-grade kernel-mode system driver.")
else()
    # SIMULATION MODE: Exclude kernel driver modules to prevent local header collision traps
    foreach(FILE_PATH ${SOURCE_FILES})
        if(FILE_PATH MATCHES "windows_driver_wrapper\\.c$")
            list(REMOVE_ITEM SOURCE_FILES ${FILE_PATH})
        endif()
    endforeach()
    
    message(STATUS "[TARGET CONFIGURATION] Preparing isolated user-space application simulator.")
endif()

# =========================================================================
# TARGET GENERATION AND INCLUSION RULES
# =========================================================================

# Build project target node from filtered list of files
add_executable(enclave_system_node ${SOURCE_FILES})

# Configure default internal search directory mappings
target_include_directories(enclave_system_node PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/intel
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/amd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/network
    ${CMAKE_CURRENT_SOURCE_DIR}/src/boot
)

# KERNEL MODE ONLY: Bind Windows Kits development headers out-of-band to resolve ntddk paths
if(DEFINED ENCLAVE_COMPILE_BARE_METAL)
    target_include_directories(enclave_system_node PUBLIC
        "C:/Program Files (x86)/Windows Kits/10/Include/10.0.28000.1839/km"
        "C:/Program Files (x86)/Windows Kits/10/Include/10.0.28000.1839/shared"
    )
endif()

# =========================================================================
# MASTER COMPILER COMPLIANCE AND FLAG MANAGEMENT
# =========================================================================

if(MSVC)
    # /WX  = Treat Warnings as Errors
    # /W4  = Enable High-Level Production Compiler Diagnostic Warnings
    # /GR- = Disable Runtime Type Information (RTTI) to comply with low-level targets
    # /GS- = Disable Buffer Security Checks for clean out-of-band environments
    # /wd4201 = Suppress nameless struct/union extension warning metrics
    # /wd4702 = Suppress unreachable dead-code optimization alerts to fix build halt
    target_compile_options(enclave_system_node PRIVATE 
        /W4 /WX /GR- /EHsc /GS- /wd4201 /wd4702
    )
    
    # Enable macro definitions to layout packet cache horizons safely
    target_compile_definitions(enclave_system_node PRIVATE 
        ENCLAVE_DEBUG_MODE=1
        ENCLAVE_SOFTWARE_NETWORK_TRANSIT=1
        MASTER_ARCHITECT="Frederick_Joseph_Lombardi"
    )
    
    # Append low-level platform properties to handle link constraints
    if(DEFINED ENCLAVE_COMPILE_BARE_METAL)
        set_target_properties(enclave_system_node PROPERTIES SUFFIX ".sys")
        # Direct MSVC linker to build a native kernel module infrastructure
        target_link_options(enclave_system_node PRIVATE "/DRIVER" "/SUBSYSTEM:NATIVE")
    endif()
endif()
