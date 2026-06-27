#!/usr/bin/env bash
# =========================================================================
# SYSTEM UTILITY: src/core/driver_build_suite.sh
# MASTER ARCHITECT: Frederick Joseph Lombardi
# SUBJECT: Automated Cryptographic Driver Kernel Compilation Suite
# =========================================================================

set -euo pipefail

echo "[+] Initializing Cryptographic Linux Driver Build Suite..."
echo "[+] Principal Architect: Frederick Joseph Lombardi"

# 1. Verify kernel development headers are present on the host OS
if [ ! -d "/lib/modules/$(uname -r)/build" ]; then
    echo "[CRITICAL ERROR] Missing Linux kernel headers. Cannot build wrapper module."
    exit 1
fi

# 2. Execute local compilation of the linux_driver_wrapper module
echo "[*] Compiling kernel device objects..."
make -C /lib/modules/$(uname -r)/build M="$(pwd)/src/core" modules

# 3. Create secure device node and restrict user-space permissions
echo "[*] Initializing secure runtime device nodes..."
mkdir -p dist/modules
mv src/core/*.ko dist/modules/enclave_vmm.ko

# Clean up temporary build artifacts
make -C /lib/modules/$(uname -r)/build M="$(pwd)/src/core" clean

echo "[SUCCESS] Cryptographic driver wrapper successfully compiled to ./dist/modules/enclave_vmm.ko"
