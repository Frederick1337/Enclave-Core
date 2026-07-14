cmake_minimum_required(VERSION 3.20)

project(EnclaveCoreNetworkTemp LANGUAGES C CXX ASM_NASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# =========================================================================
# GLOBAL SOURCE FILE EXPLORATION MATRIX
# =========================================================================
file(GLOB_RECURSE ALL_CPP_FILES "src/*.cpp")
file(GLOB_RECURSE ALL_C_FILES   "src/*.c")
file(GLOB_RECURSE ALL_ASM_FILES "src/common/*.asm" "src/boot/*.asm")

# =========================================================================
# CONDITIONAL MULTI-BUILD TARGET BRANCHING
# =========================================================================

if(DEFINED ENCLAVE_COMPILE_BARE_METAL)
    message(STATUS "[MULTI-BUILD] Isolating target for Ring 0 Kernel-Mode Driver...")

    # 1. HARDWARE ISOLATION: Natively extract only low-level kernel driver code
    # Completely exclude user-space modules to prevent std::thread / std::ios_base link explosions
    set(DRIVER_SOURCES "")
    foreach(FILE_PATH ${ALL_C_FILES} ${ALL_ASM_FILES} ${ALL_CPP_FILES})
        # Explicitly pull in only your low-level hardware virtualization layers
        if(FILE_PATH MATCHES "windows_driver_wrapper\\.c$" OR 
           FILE_PATH MATCHES "amd_gate\\.asm$" OR 
           FILE_PATH MATCHES "bootloader\\.asm$" OR
           FILE_PATH MATCHES "svm_core\\.cpp$" OR
           FILE_PATH MATCHES "vmx_core\\.cpp$" OR
           FILE_PATH MATCHES "ept_handler\\.cpp$" OR
           FILE_PATH MATCHES "npt_handler\\.cpp$")
            list(APPEND DRIVER_SOURCES ${FILE_PATH})
        endif()
    endforeach()

    # 2. CREATE KERNEL TARGET
    add_executable(enclave_system_node ${DRIVER_SOURCES})
    set_target_properties(enclave_system_node PROPERTIES SUFFIX ".sys")

    # 3. CONFIGURE KERNEL INCLUDE PATHS
    set(WDK_BASE_PATH "C:/Program Files (x86)/Windows Kits/10/Include/10.0.26100.0")
    target_include_directories(enclave_system_node PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        "${WDK_BASE_PATH}/km" 
        "${WDK_BASE_PATH}/shared"
    )

    # 4. PRIVILEGED LINKER MAPS
    if(MSVC)
        target_compile_options(enclave_system_node PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:/W4 /WX /GR- /EHsc /GS- /wd4201 /wd4702>)
        
        set(CMAKE_C_STANDARD_LIBRARIES "")
        set(CMAKE_CXX_STANDARD_LIBRARIES "")
        
        target_link_options(enclave_system_node PRIVATE 
            "/DRIVER" 
            "/SUBSYSTEM:NATIVE" 
            "/MANIFEST:NO"
            "/NODEFAULTLIB"
            "/ENTRY:DriverEntry"
        )
        
        target_link_libraries(enclave_system_node PRIVATE 
            "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/km/x64/ntoskrnl.lib"
            "C:/Program Files (x86)/Windows Kits/10/Lib/10.0.26100.0/km/x64/BufferOverflowK.lib"
        )
    endif()

else()
    message(STATUS "[MULTI-BUILD] Isolating target for User-Space Simulation Executable...")

    # 1. USER-SPACE ISOLATION: Strip out Ring 0 driver boot files to prevent conflicts
    set(SIMULATOR_SOURCES "")
    foreach(FILE_PATH ${ALL_CPP_FILES} ${ALL_C_FILES} ${ALL_ASM_FILES})
        if(NOT FILE_PATH MATCHES "windows_driver_wrapper\\.c$" AND 
           NOT FILE_PATH MATCHES "linux_driver_wrapper\\.c$" AND 
           NOT FILE_PATH MATCHES "linux_module_wrapper\\.c$" AND
           NOT FILE_PATH MATCHES "kernel_cpp_runtime\\.cpp$")
            list(APPEND SIMULATOR_SOURCES ${FILE_PATH})
        endif()
    endforeach()

    # 2. CREATE USER-SPACE TARGET
    add_executable(enclave_system_node ${SIMULATOR_SOURCES})

    # 3. CONFIGURE SIMULATOR INCLUDE PATHS
    target_include_directories(enclave_system_node PUBLIC 
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/common
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/intel
        ${CMAKE_CURRENT_SOURCE_DIR}/src/core/amd
        ${CMAKE_CURRENT_SOURCE_DIR}/src/network
        ${CMAKE_CURRENT_SOURCE_DIR}/src/boot
    )

    # 4. APPLICATION FLAGS
    if(MSVC)
        target_compile_options(enclave_system_node PRIVATE $<$<COMPILE_LANGUAGE:C,CXX>:/W4 /WX /EHsc /wd4201 /wd4702>)
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(enclave_system_node PRIVATE $<$<COMPILE_LANGUAGE:C>:-Wall -Wextra -Werror -mrdrnd> $<$<COMPILE_LANGUAGE:CXX>:-Wall -Wextra -Werror -std=c++20 -mrdrnd>)
    endif()

endif()

# Global macro definitions applied safely across all compilation units
target_compile_definitions(enclave_system_node PRIVATE 
    ENCLAVE_DEBUG_MODE=1
    ENCLAVE_SOFTWARE_NETWORK_TRANSIT=1
    MASTER_ARCHITECT="Frederick_Joseph_Lombardi"
    $<$<PLATFORM_ID:Windows>:WIN32 _WINDOWS>
    $<$<AND:$<PLATFORM_ID:Windows>,$<COMPILE_LANGUAGE:C,CXX>>:_AMD64_ _X64_ AMD64>
)
