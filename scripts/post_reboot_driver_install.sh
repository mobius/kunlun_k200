#!/usr/bin/env bash
# Run AFTER reboot when kunlun was stuck in "Unloading".
# Installs patched repo driver and prints knobs. Optional: REGRESSION=1
set -euo pipefail
cd "$(dirname "$0")/.."

echo "=== kernel $(uname -r) ==="
if grep -q 'Unloading' /proc/modules 2>/dev/null && grep -q '^kunlun ' /proc/modules; then
  echo "ERROR: kunlun still 'Unloading' — reboot required again."
  exit 1
fi

echo "=== install patched driver ==="
# Prefer make path; falls back to script if make already root
ROOT="$(pwd)"
if sudo -n /usr/bin/make -C "$ROOT" driver-install; then
  :
elif sudo /usr/bin/make -C "$ROOT" driver-install; then
  :
else
  echo "make driver-install failed; try: sudo bash scripts/install_driver.sh"
  exit 1
fi

echo "=== verify ==="
modinfo -F srcversion,vermagic,filename kunlun
for p in kl1_p2p_stub kl1_dma_direct kl1_bounce_pipe; do
  f=/sys/module/kunlun/parameters/$p
  if [[ -f $f ]]; then echo "$p=$(cat $f)"; else echo "$p=MISSING"; fi
done
ls /dev/xpu* 2>/dev/null | wc -l | xargs -I{} echo "xpu nodes: {}"
xpu_smi 2>/dev/null | head -18 || true

if [[ "${REGRESSION:-0}" == "1" ]]; then
  export LD_LIBRARY_PATH=/usr/local/xpu-4.33.0/lib64:$(pwd)/xdnn-ubuntu_x86_64/so${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
  make regression
fi
echo "=== done ==="
