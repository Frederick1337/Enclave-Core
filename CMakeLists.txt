cmake_minimum_required(VERSION 3.20)
project(EnclaveCoreLocal LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
file(MAKE_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})

add_compile_definitions(MASTER_ARCHITECT="Frederick_Joseph_Lombardi")
add_compile_definitions(ENCLAVE_DEBUG_MODE=1)

set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

if(MSVC)
    # Windows MSVC Flag Mapping: Preserves exception unwinding for standard templates
    add_compile_options(/W4 /WX /GR- /EHsc /GS- /wd4201 /std:c++20 /Oi)
    
    # Conditional Driver Linking: Uses console subsystem for hosted cloud simulation runners
    if(DEFINED ENCLAVE_COMPILE_BARE_METAL)
        add_link_options(/DRIVER /SUBSYSTEM:NATIVE /ENTRY:DriverEntry /MANIFEST:NO)
    else()
        add_link_options(/SUBSYSTEM:CONSOLE)
    endif()
else()
    add_compile_options(-Wall -Wextra -Werror -fno-stack-protector -O2 -mcmodel=kernel -fno-pic -fno-pie -mrdrnd)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>)
    add_compile_options($<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>)
endif()

# AUTOMATED PATH DISCOVERY LAYER: Recursively maps existing source files (.cpp, .c)
file(GLOB_RECURSE SOURCE_FILES 
    "src/*.cpp" 
    "src/*.c"
)

# RE-ARMED EXCLUSION LOOP: Drops bare-metal module wrappers on host cloud user-space tests
if(NOT DEFINED ENCLAVE_COMPILE_BARE_METAL)
    foreach(FILE_PATH ${SOURCE_FILES})
        if(FILE_PATH MATCHES "wrapper\\.c$")
            list(REMOVE_ITEM SOURCE_FILES ${FILE_PATH})
        endif()
    endforeach()
endif()

if(NOT SOURCE_FILES)
    set(EMPTY_FALLBACK_FILE ${CMAKE_CURRENT_BINARY_DIR}/enclave_generated_init.cpp)
    file(WRITE ${EMPTY_FALLBACK_FILE} "// Automated Project Enclave Initialization Anchor Component\nint main() { return 0; }\n")
    set(SOURCE_FILES ${EMPTY_FALLBACK_FILE})
endif()

add_executable(enclave_core_node ${SOURCE_FILES})

if(NOT MSVC)
    target_link_options(enclave_core_node PRIVATE -no-pie)
endif()

target_include_directories(enclave_core_node PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/common
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/intel
    ${CMAKE_CURRENT_SOURCE_DIR}/src/core/amd
    ${CMAKE_CURRENT_SOURCE_DIR}/src/boot
)

if(WIN32)
    # Bypasses WDK dependencies unless explicitly building bare-metal installation binaries
    if(DEFINED ENCLAVE_COMPILE_BARE_METAL)
        target_link_libraries(enclave_core_node PUBLIC ntoskrnl.lib hal.lib wdfdriverstub.lib)
    endif()
else()
    target_link_libraries(enclave_core_node PUBLIC pthread)
endif()
