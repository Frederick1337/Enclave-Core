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
    "src/boot/*.asm"
)

# =========================================================================
# OPERATING SYSTEM SOURCE FILE FILTER MATRIX
# =========================================================================

# Step 1: Filter out platform-specific bootstrapping code based on compile environments
if(WIN32)
    foreach(FILE_PATH ${SOURCE_FILES})
        if(FILE_PATH MATCHES "linux_module_wrapper\\.c$" OR FILE_PATH MATCHES "linux_driver_wrapper\\.c$")
            list(REMOVE_ITEM SOURCE_FILES ${FILE_PATH})
        endif()
    endforeach()
else()
    foreach(FILE_PATH ${SOURCE_FILES})
        if(FILE_PATH MATCHES "windows_driver_wrapper\\.c$")
            list(REMOVE_ITEM SOURCE_FILES ${FILE_PATH})
        endif()
        if(FILE_PATH MATCHES "amd_gate\\.asm$")
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
        if(FILE_PATH MATCHES "windows_driver_wrapper\\.c$" OR FILE_PATH MATCHES "linux_module_wrapper\\.c$" OR FILE_PATH MATCHES "linux_driver_wrapper\\.c$")
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

# KERNEL MODE ONLY: Bind Windows Kits development headers dynamically
if(DEFINED ENCLAVE_COMPILE_BARE_METAL AND WIN32)
    # Target path 1: Standard WDK deployment location verified by WinGet installer
    set(WDK_BASE_PATH "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0")
    
    # Target path 2: Disjoint ExtensionSDKs location used by standalone WDK bundles
    set(WDK_EXT_PATH "C:/Program Files (x86)/Windows Kits/10/ExtensionSDKs/WindowsDriverKit/10.0.26100.0/Include/km")

    # Evaluate the file paths dynamically and apply the active directory to your build includes
    if(EXISTS "${WDK_BASE_PATH}/km/ntddk.h")
        set(WDK_INCLUDE_DIR "${WDK_BASE_PATH}/km")
        set(WDK_SHARED_DIR "${WDK_BASE_PATH}/shared")
    else()
        set(WDK_INCLUDE_DIR "C:/Program Files (x86)/Windows Kits/10/ExtensionSDKs/WindowsDriverKit/10.0.26100.0/Include/km")
        set(WDK_SHARED_DIR "C:/Program Files (x86)/Windows Kits/10/ExtensionSDKs/WindowsDriverKit/10.0.26100.0/Include/shared")
    endif()

    message(STATUS "[WDK MATRIX MATCH] Directing include paths straight to: ${WDK_INCLUDE_DIR}")

    target_include_directories(enclave_system_node PUBLIC
        "${WDK_INCLUDE_DIR}"
        "${WDK_SHARED_DIR}"
    )
endif()

# =========================================================================
# MASTER COMPILER COMPLIANCE AND FLAG MANAGEMENT
# =========================================================================

if(MSVC)
    # FIXED SCOPED COMPILER OPTIONS: Wrapped in compile language condition blocks.
    target_compile_options(enclave_system_node PRIVATE 
        $<$<COMPILE_LANGUAGE:C,CXX>:/W4 /WX /GR- /EHsc /GS- /wd4201 /wd4702>
    )
    
    # Append low-level platform properties to handle link constraints
    if(DEFINED ENCLAVE_COMPILE_BARE_METAL)
        set_target_properties(enclave_system_node PROPERTIES SUFFIX ".sys")
        
        # STRIP DEFAULT WIN32 LIBRARIES: Prevents user-space dependency injection faults
        set(CMAKE_C_STANDARD_LIBRARIES "")
        set(CMAKE_CXX_STANDARD_LIBRARIES "")
        
        # FIXED DRIVER LINK SETTINGS: Strips /MANIFEST and clears out user-space default libraries
        target_link_options(enclave_system_node PRIVATE 
            "/DRIVER" 
            "/SUBSYSTEM:NATIVE" 
            "/MANIFEST:NO"
            "/NODEFAULTLIB"
            "/ENTRY:DriverEntry"
        )
        
        # Ingest required kernel entry libraries from the WDK
        target_link_libraries(enclave_system_node PRIVATE 
            "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/km/x64/ntoskrnl.lib"
            "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/km/x64/BufferOverflowK.lib"
        )
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # LANGUAGE ISOLATION FIXED: Pass specific dialect definitions exclusively to C or CXX source streams
    target_compile_options(enclave_system_node PRIVATE 
        $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra -Werror -mrdrnd>
        $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Werror -std=c++20 -mrdrnd>
    )
endif()

# Global macro definitions applied safely across all compilation units
target_compile_definitions(enclave_system_node PRIVATE 
    ENCLAVE_DEBUG_MODE=1
    ENCLAVE_SOFTWARE_NETWORK_TRANSIT=1
    MASTER_ARCHITECT="Frederick_Joseph_Lombardi"
    
    # FIXED ARCHITECTURE BLOCK: Injects required kernel macros based on host system
    $<$<PLATFORM_ID:Windows>:WIN32 _WINDOWS>
    $<$<AND:$<PLATFORM_ID:Windows>,$<COMPILE_LANGUAGE:C,CXX>>:_AMD64_ _X64_ AMD64>
)
