#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
echo "=== Installing patched kunlun (kl1_*=production) ==="
sudo make driver-install
echo "=== Post-install identity ==="
modinfo -F srcversion,vermagic kunlun
for p in kl1_p2p_stub kl1_dma_direct kl1_bounce_pipe; do
  echo -n "$p="; cat /sys/module/kunlun/parameters/$p 2>/dev/null || echo missing
done
xpu_smi | head -20
echo "=== Running regression ==="
export LD_LIBRARY_PATH=/usr/local/xpu-4.33.0/lib64:$(pwd)/xdnn-ubuntu_x86_64/so
make regression
