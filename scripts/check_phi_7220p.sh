#!/usr/bin/env bash
# check_phi_7220p.sh — Intel Xeon Phi 7220P (KNL) Phase 0/1 hardware baseline
# Usage:
#   bash scripts/check_phi_7220p.sh 2>&1 | tee docs/research/$(date +%Y%m%d_%H%M%S)_phi_7220p_hw.log
set -uo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

pass()  { echo -e "${GREEN}[PASS]${NC} $*"; }
fail()  { echo -e "${RED}[FAIL]${NC} $*"; }
warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
info()  { echo -e "[INFO] $*"; }
header(){ echo; echo "=== $* ==="; }

# Expected anchors for this machine (7220P)
EXPECT_BDF_MAIN="${PHI_BDF_MAIN:-8b:00.0}"
EXPECT_BDF_AUX="${PHI_BDF_AUX:-8b:00.1}"
EXPECT_VENDOR="0x8086"
EXPECT_DEVICE="0x2260"
EXPECT_DEVICE_AUX="0x2264"
EXPECT_SUBSYS="0x7498"

PASS_N=0
FAIL_N=0
WARN_N=0

record_pass() { pass "$@"; PASS_N=$((PASS_N + 1)); }
record_fail() { fail "$@"; FAIL_N=$((FAIL_N + 1)); }
record_warn() { warn "$@"; WARN_N=$((WARN_N + 1)); }

sysfs_main="/sys/bus/pci/devices/0000:${EXPECT_BDF_MAIN}"
sysfs_aux="/sys/bus/pci/devices/0000:${EXPECT_BDF_AUX}"

# ─── Host ───
header "Host / OS"
info "hostname: $(hostname)"
info "date: $(date -Is 2>/dev/null || date)"
info "kernel: $(uname -r)"
if [[ -f /etc/os-release ]]; then
  # shellcheck source=/dev/null
  . /etc/os-release
  info "os: ${PRETTY_NAME:-unknown}"
fi
info "cpu: $(lscpu 2>/dev/null | awk -F: '/Model name|型号名称|ModelName/{print $2; exit}' | sed 's/^[[:space:]]*//')"
info "nproc: $(nproc)"
free -h | head -2
echo

# ─── Locate 7220P ───
header "Locate Phi 7220P (PCI)"

if [[ -d "$sysfs_main" ]]; then
  record_pass "sysfs main present: $sysfs_main"
else
  record_fail "sysfs main missing: $sysfs_main"
fi

if [[ -d "$sysfs_aux" ]]; then
  record_pass "sysfs aux present: $sysfs_aux"
else
  record_warn "sysfs aux missing: $sysfs_aux"
fi

echo
info "lspci lines matching Intel 2260/2264 or Phi:"
lspci -nn 2>/dev/null | grep -E "2260|2264|Phi|Co-processor|1d22:3684|66a1" || true
echo

read_sysfs() {
  local path="$1"
  if [[ -r "$path" ]]; then
    cat "$path" 2>/dev/null | tr -d '\n'
  else
    echo "N/A"
  fi
}

if [[ -d "$sysfs_main" ]]; then
  header "Main function ${EXPECT_BDF_MAIN} identity"
  vendor=$(read_sysfs "$sysfs_main/vendor")
  device=$(read_sysfs "$sysfs_main/device")
  subven=$(read_sysfs "$sysfs_main/subsystem_vendor")
  subdev=$(read_sysfs "$sysfs_main/subsystem_device")
  rev=$(read_sysfs "$sysfs_main/revision")
  class=$(read_sysfs "$sysfs_main/class")
  enable=$(read_sysfs "$sysfs_main/enable")
  irq=$(read_sysfs "$sysfs_main/irq")
  numa=$(read_sysfs "$sysfs_main/numa_node")
  speed=$(read_sysfs "$sysfs_main/current_link_speed")
  width=$(read_sysfs "$sysfs_main/current_link_width")
  maxw=$(read_sysfs "$sysfs_main/max_link_width")
  maxs=$(read_sysfs "$sysfs_main/max_link_speed")

  info "vendor:device = ${vendor}:${device}"
  info "subsystem     = ${subven}:${subdev}"
  info "revision      = ${rev}"
  info "class         = ${class}"
  info "enable        = ${enable}"
  info "irq           = ${irq}"
  info "numa_node     = ${numa}"
  info "link          = ${speed} x${width} (max ${maxs} x${maxw})"

  [[ "$vendor" == "$EXPECT_VENDOR" && "$device" == "$EXPECT_DEVICE" ]] \
    && record_pass "device ID matches 7220P anchor ${EXPECT_VENDOR}:${EXPECT_DEVICE}" \
    || record_fail "device ID ${vendor}:${device} != ${EXPECT_VENDOR}:${EXPECT_DEVICE}"

  [[ "$subdev" == "$EXPECT_SUBSYS" ]] \
    && record_pass "subsystem device ${EXPECT_SUBSYS}" \
    || record_warn "subsystem device ${subdev} (expected ${EXPECT_SUBSYS})"

  if [[ "$width" == "16" ]]; then
    record_pass "link width x16"
  else
    record_warn "link width x${width} (expected x16)"
  fi

  if [[ "$enable" == "1" ]]; then
    record_pass "PCI enable=1"
  else
    record_warn "PCI enable=${enable} (card not enabled; Phase 1 needed)"
  fi

  if [[ -e "$sysfs_main/driver" ]]; then
    drv=$(basename "$(readlink -f "$sysfs_main/driver")")
    record_pass "driver bound: $drv"
  else
    record_warn "no kernel driver bound (expected until Phase 1)"
  fi

  echo
  info "resource (BARs):"
  if [[ -r "$sysfs_main/resource" ]]; then
    nl -ba "$sysfs_main/resource" | head -10
    # BAR2 often 32G for this card
    bar2=$(awk 'NR==3 {print}' "$sysfs_main/resource")
    info "BAR2 line: $bar2"
    # parse start end if non-zero
    start=$(echo "$bar2" | awk '{print $1}')
    end=$(echo "$bar2" | awk '{print $2}')
    if [[ "$start" != "0x0000000000000000" && -n "$start" ]]; then
      # size in GiB via python for reliability
      gsize=$(python3 - <<PY
start=int("${start}", 16)
end=int("${end}", 16)
print(f"{(end-start+1)/(1024**3):.1f}")
PY
)
      info "BAR2 size ≈ ${gsize} GiB"
      python3 - <<PY
g=float("${gsize}")
import sys
sys.exit(0 if g >= 16 else 1)
PY
      if [[ $? -eq 0 ]]; then
        record_pass "large prefetch BAR present (~${gsize} GiB)"
      else
        record_warn "BAR2 size ~${gsize} GiB (expected large window)"
      fi
    fi
  fi

  echo
  info "device path: $(readlink -f "$sysfs_main" 2>/dev/null || true)"
fi

if [[ -d "$sysfs_aux" ]]; then
  header "Aux function ${EXPECT_BDF_AUX}"
  av=$(read_sysfs "$sysfs_aux/vendor")
  ad=$(read_sysfs "$sysfs_aux/device")
  info "vendor:device = ${av}:${ad}"
  [[ "$ad" == "$EXPECT_DEVICE_AUX" ]] \
    && record_pass "aux device ${EXPECT_DEVICE_AUX}" \
    || record_warn "aux device ${ad}"
fi

# ─── Legacy MIC / MPSS (should be absent for KNL path) ───
header "Legacy KNC stack (MPSS 3.x) — informational"
if command -v micinfo &>/dev/null; then
  record_warn "micinfo present (KNC-era tool); 7220P may not use it"
  micinfo 2>&1 | head -40 || true
else
  info "micinfo: not installed (OK for KNL-first path)"
fi
if command -v micctrl &>/dev/null; then
  record_warn "micctrl present"
  micctrl --status 2>&1 | head -20 || true
else
  info "micctrl: not installed"
fi
if systemctl is-active mpss &>/dev/null; then
  record_warn "mpss service active"
else
  info "mpss service: not active"
fi
ls /dev/mic* 2>/dev/null && record_warn "found /dev/mic*" || info "/dev/mic*: none"
lsmod 2>/dev/null | grep -E '^mic\b|^mpss' || info "mic/mpss modules: not loaded"
modinfo mic 2>&1 | head -3 || true

# ─── Topology / neighbors ───
header "PCIe neighborhood (switch / peer GPUs)"
info "lspci -tv snippet (bus 80 branch if present):"
lspci -tv 2>/dev/null | grep -E "80:|8a:|8b:|86:|87:|Phi|2260|Vega|MI50|Baidu|Kunlun" || \
  lspci -tv 2>/dev/null | head -5
echo
info "Accelerators / interesting endpoints:"
lspci -nn 2>/dev/null | grep -iE "2260|2264|vega|66a1|1d22:3684|mellanox|processing accel|3d|vga" || true

# ─── Kernel messages ───
header "Kernel messages (8b:00 / 2260 / phi / mic)"
if command -v journalctl &>/dev/null; then
  journalctl -b -k --no-pager 2>/dev/null | grep -iE "8b:00|2260|2264|xeon phi| mic |mpss" | tail -40 \
    || info "no matching journal lines"
else
  dmesg 2>/dev/null | grep -iE "8b:00|2260|2264|xeon phi| mic " | tail -40 || true
fi

# ─── Coexistence load (informational) ───
header "Coexisting accelerators (load context)"
if command -v xpu_smi &>/dev/null; then
  info "xpu_smi (first 20 lines):"
  xpu_smi 2>/dev/null | head -20 || true
else
  info "xpu_smi: not in PATH"
fi
if command -v rocm-smi &>/dev/null; then
  info "rocm-smi concise:"
  rocm-smi 2>/dev/null | head -25 || true
else
  info "rocm-smi: not in PATH"
fi
info "loadavg: $(cat /proc/loadavg)"

# ─── Manual checklist ───
header "Manual checklist (not auto-verified)"
info "[ ] Auxiliary power connectors seated (6/8-pin as required)"
info "[ ] Passiveflow for 300W passive card adequate"
info "[ ] Full-load power budget vs PSU (Phi + K200 + MI50 mutex)"
info "[ ] BIOS: Above 4G Decoding / IOMMU / large BAR reviewed for Phase 1"

# ─── Summary ───
header "Summary"
echo "  PASS: $PASS_N"
echo "  WARN: $WARN_N"
echo "  FAIL: $FAIL_N"
echo
echo "Model: Intel Xeon Phi 7220P (KNL PCIe) — do NOT treat as 7120P/KNC MPSS3 path."
echo "Plan:  docs/plan/20260805-xeon-phi-7220p-bringup.md"
echo "Discover: python3 scripts/phi_discover.py"
echo

if [[ "$FAIL_N" -gt 0 ]]; then
  exit 2
fi
exit 0
