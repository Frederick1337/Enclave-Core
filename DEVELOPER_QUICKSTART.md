# 🚀 Developer Onboarding: Integrating with Project Enclave (Local Core)

Welcome to Project Enclave. This guide walks you through adapting, compiling, and deploying your own separate applications to use our Ring -1 bare-metal memory-shuffling ecosystem. 

By following these steps, you can immunize **any** custom data structure or variable type in your program against memory scrapers, pointer scanners, and physical PCIe DMA sniffing cards with zero modifications required to the underlying hypervisor core.

---

## 🛠️ Step 1: Include the Interface Headers in Your Project

You do not write your application code inside our repository. Instead, simply copy the public interface files from our repository's common directory and drop them into your own separate project's source tree:

* Copy **`src/common/enclave_integration.h`** into your local project directory.
* Copy **`src/common/enclave_hypercall.h`** into your local project directory.

At the very top of your application's source code files (e.g., `main.cpp` or `game_client.cpp`), include the master templates to open the hypervisor communication channels:

```cpp
#include "enclave_integration.h"
#include "enclave_hypercall.h"
#include <iostream>
```

---

## 🔒 Step 2: Establish the Early-Boot Attestation Handshake

Before your program loads any sensitive data, configuration matrices, or secret keys, you must query our running bare-metal hypervisor to ensure the local host machine is safe and virtualized. Add this validation loop right at the entry point of your program (`main`):

```cpp
int main() {
    // Ping the Ring -1 VMM using Command Vector 0x01
    // Pass the master authorization token down to the CPU registers
    if (EnclaveHypercallGate::IssueHypercall(0x01, 0x55AAFJLOMBARDI) != 0xAA) {
        std::cerr << "[⚠️ FATAL] Environment Unsecure! Project Enclave Hypervisor is missing.\n";
        std::cerr << "Ensure the bare-metal kernel driver is loaded at boot before running this app.\n";
        std::exit(EXIT_FAILURE); 
    }
    
    std::cout << "[🛡️ SECURE] Enclave attestation passed. Initializing application enclaves...\n";
    
    // Your secure program logic goes here
    return 0;
}
```

---

## 🧬 Step 3: Wrap and Protect Your Variables

You are **not** limited to specific variable configurations. Because our SDK leverages standard **C++ Templates**, you can protect any data type (integers, text strings, coordinate vectors, or massive structural rows) automatically.

### 1. Define Any Custom Structure
Create your variables or object layouts exactly as you normally would:

```cpp
struct ProtectedSystemState {
    uint32_t variable_id;
    float boundary_x;
    float boundary_y;
    uint64_t secure_token_mask;
};
```

### 2. Pass Your Structure into the Open Template Slot
Instead of instantiating a naked, static variable in RAM, drop your data type inside the angled brackets `< >` of our `ProtectedVariable` template. Bind it to the hypervisor core to engage the **Dynamic Namespace Variable Switching Engine**:

```cpp
// Instantiate your baseline data object
ProtectedSystemState raw_data = { 9012, 142.5f, -88.2f, 0xABCDE12345 };

// Pass your custom data type into the template slot
EnclaveSDK::ProtectedVariable<ProtectedSystemState> secure_matrix;

// Bind the container to activate microsecond memory shuffling
secure_matrix.BindToEnclaveCore(&raw_data);
```

---

## 🏎️ Step 4: Interact with Data via Safe Getters and Setters

To prevent application page faults, you must never reference or pointer-scrape wrapped variables directly. Interact with your secure memory enclaves exclusively through thread-safe atomic getters and setters:

### To Read Secure Values:
When you invoke `.Get()`, the SDK reads from the isolated CPU register cache, un-scrambling the layout for a microsecond fraction to give you a pristine snapshot without exposing the raw RAM structure to external attackers:

```cpp
ProtectedSystemState live_snapshot = secure_matrix.Get();
std::cout << "Active Variable ID: " << live_snapshot.variable_id << "\n";
```

### To Update Secure Values:
When you alter parameters, modify your snapshot copy and push it back using `.Set()`. The **Polymorphic Sync Engine** immediately intercepts the memory footprint, shuffling its physical RAM offsets based on the machine's true hardware `RDRAND` entropy and unique processor stepping matrices:

```cpp
ProtectedSystemState updated_snapshot = secure_matrix.Get();
updated_snapshot.boundary_x += 5.5f; // Modify a parameter

secure_matrix.Set(updated_snapshot); // Re-scramble across physical RAM instantly
```

---

## 🔨 Step 5: Adapt Your Compiler Build Configuration

To compile your application cleanly alongside our integration templates, you must update your local build project variables to support the **C++20 Standard** or higher. 

### If building with CMake (`CMakeLists.txt`):
Add these configuration lines right below your project declaration to enforce C++20 standard requirements and turn off unsafe runtime code RTTI expansions:

```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(MSVC)
    add_compile_options(/W4 /WX /GR- /EHs-c- /GS-)
else()
    add_compile_options(-Wall -Wextra -Werror -fno-rtti -fno-exceptions)
endif()

# Link your app binary targets
add_executable(my_protected_app src/main.cpp)
```

### If building with Visual Studio (Windows Desktop):
1. Right-click your application project in the **Solution Explorer** and select **Properties**.
2. Navigate to **Configuration Properties > General**.
3. Locate **C++ Language Standard** and change the drop-down to **ISO C++20 Standard (/std:c++20)**.
4. Navigate to **C/C++ > Language** and set **Enable Run-Time Type Information** to **No (/GR-)**.
5. Click **Apply** and select **Build Solution**.
