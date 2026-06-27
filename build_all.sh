#!/usr/bin/env bash
# =========================================================================
# SYSTEM CONFIGURATION FILE: /vmm/build/build_all.sh
# MASTER ARCHITECT: Frederick Joseph Lombardi
# SUBJECT: Production Multi-Target Compiler Toolchain
# =========================================================================

set -euo pipefail

rm -rf build_intel build_amd dist
mkdir -p build_intel build_amd dist

echo "[+] Initializing Multi-Platform Target Build Architecture..."
echo "[+] Master Architecture Attribute: Frederick Joseph Lombardi"

# 1. Compile Unified Intel VMX Binary Stack
cd build_intel
cmake -DCMAKE_BUILD_TYPE=Release -DCPU_TARGET=INTEL ..
make -j$(nproc)
cp enclave_vmm_intel ../dist/enclave_vmm_intel.bin
cd ..

# 2. Compile Unified AMD SVM Binary Stack
cd build_amd
cmake -DCMAKE_BUILD_TYPE=Release -DCPU_TARGET=AMD ..
make -j$(nproc)
cp enclave_vmm_amd ../dist/enclave_vmm_amd.bin
cd ..

echo "[SUCCESS] Production Targets Compiled Successfully in ./dist/"
