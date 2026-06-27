#!/usr/bin/env bash
set -euo pipefail

if [[ $EUID -ne 0 ]]; then
   echo "[FATAL] Root administrator privileges required." 
   exit 1
fi

echo "[1/3] Enumerating CPU Virtualization Configurations..."
CPU_SIGNATURE=$(grep -m 1 'vendor_id' /proc/cpuinfo | awk '{print $3}')

if [ "$CPU_SIGNATURE" == "GenuineIntel" ]; then
    BOOT_PARAMETERS="intel_iommu=on iommu=pt kvm_intel.nested=1"
elif [ "$CPU_SIGNATURE" == "AuthenticAMD" ]; then
    BOOT_PARAMETERS="amd_iommu=on iommu=pt kvm_amd.nested=1"
else
    echo "[ERROR] Silicon platform is unsupported."
    exit 1
fi

echo "[2/3] Writing Hardened Kernel Infrastructure Parameters..."
if [ -d /etc/default/grub.d ]; then
    echo "GRUB_CMDLINE_LINUX_DEFAULT=\"\$GRUB_CMDLINE_LINUX_DEFAULT ${BOOT_PARAMETERS}\"" > /etc/default/grub.d/99-enclave-vmm.cfg
else
    sed -i "s/GRUB_CMDLINE_LINUX_DEFAULT=\"[^\"]*/& ${BOOT_PARAMETERS}/" /etc/default/grub
fi
update-grub

echo "[3/3] Setting Hardware Layer Resource Hardening..."
cat <<EOF > /etc/sysctl.d/99-enclave-hardware.conf
vm.max_map_count=524288
kernel.randomize_va_space=2
fs.protected_hardlinks=1
EOF
sysctl --system

mkdir -p /etc/enclave_vmm/
cp src/config/iommu_policy.cfg /etc/enclave_vmm/iommu_policy.cfg

echo "[DEPLOYMENT SUCCESSFUL] Host server prepared for execution routing."
