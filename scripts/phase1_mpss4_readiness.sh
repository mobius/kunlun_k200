#!/usr/bin/env bash
# phase1_mpss4_readiness.sh — Gate check before/during MPSS 4 (KNL/7220P) install
# Does NOT install packages. Safe to run on Ubuntu 22.04 work OS or Rocky target OS.
set -uo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'
pass() { echo -e "${GREEN}[PASS]${NC} $*"; }
fail() { echo -e "${RED}[FAIL]${NC} $*"; }
warn() { echo -e "${YELLOW}[WARN]${NC} $*"; }
info() { echo "[INFO] $*"; }
header() { echo; echo "=== $* ==="; }

P=0; F=0; W=0
ok() { pass "$@"; P=$((P+1)); }
bad() { fail "$@"; F=$((F+1)); }
wa() { warn "$@"; W=$((W+1)); }

REPO_ROOT="$(cd "$(dirname "$0")/.." && pwd)"
MPSS_VENDOR="${MPSS_VENDOR_DIR:-$REPO_ROOT/third_party/mpss4}"
JJ_MPSS="${JJ_MPSS_DIR:-$REPO_ROOT/third_party/jjkeijser-mpss}"

header "Host context"
info "host=$(hostname) kernel=$(uname -r)"
if [[ -f /etc/os-release ]]; then
  # shellcheck source=/dev/null
  . /etc/os-release
  info "os=${PRETTY_NAME:-?} id=${ID:-?}"
fi
KREL="$(uname -r)"
KMAJOR="${KREL%%.*}"
KREST="${KREL#*.}"
KMINOR="${KREST%%.*}"
info "kernel major.minor=${KMAJOR}.${KMINOR}"

# Kernel gate: MPSS4 modules historically ≤ ~4.18 (RHEL8.3 line)
if [[ "$KMAJOR" -eq 4 && "$KMINOR" -le 18 ]]; then
  ok "kernel ${KREL} is in historical MPSS4 support band (≤4.18)"
elif [[ "$KMAJOR" -lt 4 ]]; then
  wa "kernel ${KREL} is very old; may work with stock Intel 4.4.1"
else
  bad "kernel ${KREL} is newer than MPSS4 community port target — use Rocky 8 / 4.18 dual-boot"
fi

# Distro hint
if [[ "${ID:-}" =~ (rocky|centos|rhel|almalinux) ]]; then
  ok "RHEL-family distro (${ID}) — preferred for MPSS4 RPMs"
elif [[ "${ID:-}" == "ubuntu" ]]; then
  wa "Ubuntu work OS — OK for PCI checks; install MPSS4 on Rocky 8 target, not here"
else
  wa "distro ${ID:-unknown} — prefer Rocky/CentOS 8 for install"
fi

header "7220P PCI (must already pass check_phi_7220p.sh)"
if [[ -d /sys/bus/pci/devices/0000:8b:00.0 ]]; then
  v=$(cat /sys/bus/pci/devices/0000:8b:00.0/vendor)
  d=$(cat /sys/bus/pci/devices/0000:8b:00.0/device)
  if [[ "$v" == "0x8086" && "$d" == "0x2260" ]]; then
    ok "found 8086:2260 at 8b:00.0 (mic_x200 host ID)"
  else
    bad "8b:00.0 is ${v}:${d}, expected 8086:2260"
  fi
else
  bad "0000:8b:00.0 missing — card not visible"
fi
if [[ -d /sys/bus/pci/devices/0000:8b:00.1 ]]; then
  d1=$(cat /sys/bus/pci/devices/0000:8b:00.1/device)
  [[ "$d1" == "0x2264" ]] && ok "found 8086:2264 aux (mic_x200_dma)" || wa "aux device $d1"
else
  wa "8b:00.1 missing"
fi

header "Driver bind state (Phase 1 goals)"
for bdf in 0000:8b:00.0 0000:8b:00.1; do
  if [[ -e /sys/bus/pci/devices/$bdf/driver ]]; then
    drv=$(basename "$(readlink -f /sys/bus/pci/devices/$bdf/driver)")
    ok "$bdf driver=$drv"
  else
    wa "$bdf unbound (need mic_x200 / mic_x200_dma after install)"
  fi
done
if [[ -d /sys/class/mic ]]; then
  ok "/sys/class/mic exists: $(ls /sys/class/mic | tr '\n' ' ')"
else
  wa "/sys/class/mic absent (mpss modules not loaded)"
fi
command -v micctrl >/dev/null && ok "micctrl in PATH" || wa "micctrl not installed"
command -v micinfo >/dev/null && ok "micinfo in PATH" || wa "micinfo not installed"

header "Vendor tree: MPSS 4.4.1 full stack"
info "looking under: $MPSS_VENDOR"
if [[ -d "$MPSS_VENDOR" ]]; then
  ok "directory exists"
  # common layout markers
  shopt -s nullglob
  tars=("$MPSS_VENDOR"/mpss-4.4*.tar* "$MPSS_VENDOR"/**/mpss-4.4*.tar*)
  imgs=("$MPSS_VENDOR"/**/bzImage-knl* "$MPSS_VENDOR"/bzImage-knl*)
  if ((${#tars[@]})); then
    ok "found tarball(s): ${tars[*]}"
  else
    bad "no mpss-4.4*.tar* — place Intel MPSS 4.4.1 archive here"
  fi
  if ((${#imgs[@]})); then
    ok "found KNL card image marker: ${imgs[0]}"
  else
    wa "no bzImage-knl* yet (may be inside tar)"
  fi
  shopt -u nullglob
else
  bad "missing $MPSS_VENDOR — create and drop mpss-4.4.1 assets (gitignored)"
  info "  mkdir -p $MPSS_VENDOR"
fi

header "Community sources: jjkeijser/mpss (mpss4 modules)"
info "looking under: $JJ_MPSS"
if [[ -d "$JJ_MPSS/mpss4/mpss-modules" ]]; then
  ok "mpss4/mpss-modules present"
  if grep -q 'INTEL_PCI_DEVICE_2260' "$JJ_MPSS/mpss4/mpss-modules/mic/mic_x200/mic_hw.h" 2>/dev/null; then
    ok "source confirms PCI 0x2260"
  fi
else
  wa "clone patches repo:"
  info "  git clone https://github.com/jjkeijser/mpss.git $JJ_MPSS"
fi

header "Build prerequisites (on TARGET install OS)"
for pkg_hint in gcc make; do
  command -v $pkg_hint >/dev/null && ok "$pkg_hint present" || wa "$pkg_hint missing"
done
if [[ -d "/lib/modules/$KREL/build" ]]; then
  ok "kernel headers/build at /lib/modules/$KREL/build"
else
  wa "no /lib/modules/$KREL/build — install kernel-devel / linux-headers"
fi

header "AMD host caution"
if grep -qi AuthenticAMD /proc/cpuinfo 2>/dev/null; then
  wa "CPU is AMD — MPSS never officially validated; watch IOMMU/DMA if probe fails"
  info "dmesg already showed fixed DMA aliases for 8b:00 on this host"
else
  ok "Intel host CPU (better MPSS match)"
fi

header "BIOS / power manual items"
info "[ ] Above 4G Decoding enabled (32GiB BAR)"
info "[ ] IOMMU: try AMD-Vi on/off if bind fails"
info "[ ] 7220P aux power + airflow (~300W passive)"
info "[ ] Mutex full-load vs K200 + MI50"

header "Summary"
echo "  PASS=$P  WARN=$W  FAIL=$F"
echo
echo "Docs:"
echo "  docs/research/20260805-phi-7220p-phase1-mpss4.md"
echo "  docs/plan/20260805-xeon-phi-7220p-bringup.md"
echo
if [[ "$F" -gt 0 ]]; then
  echo "Next: fix FAIL items (usually: dual-boot Rocky 8 + provide mpss-4.4.1 tarball)."
  exit 2
fi
echo "Readiness soft-OK for documentation; install only on supported kernel/distro."
exit 0
