#!/usr/bin/env python3
"""Discover Intel Xeon Phi 7220P (KNL) on this host.

Unlike uni-framework (7120P/KNC grep on "Xeon Phi Co-processor"), this machine
anchors on PCI ID 8086:2260 / BDF 8b:00.0.
"""
from __future__ import annotations

import argparse
import json
import os
import sys
from dataclasses import asdict, dataclass
from pathlib import Path
from typing import List, Optional

# Anchors for G292 + 7220P (override via env)
DEFAULT_VENDOR = "0x8086"
DEFAULT_DEVICE = "0x2260"
DEFAULT_DEVICE_AUX = "0x2264"
DEFAULT_SUBSYS = "0x7498"
DEFAULT_BDF = os.environ.get("PHI_BDF_MAIN", "0000:8b:00.0")


@dataclass
class PhiDevice:
    name: str
    kind: str  # "phi_knl"
    model: str
    online: bool
    pcie_addr: str
    vendor: str
    device: str
    subsystem_device: str
    revision: str
    class_code: str
    enable: int
    driver: Optional[str]
    link_speed: str
    link_width: str
    numa_node: int
    bar2_gib: Optional[float]
    notes: str


def _read(path: Path) -> Optional[str]:
    try:
        return path.read_text().strip()
    except OSError:
        return None


def _bar2_gib(resource_path: Path) -> Optional[float]:
    try:
        lines = resource_path.read_text().splitlines()
        if len(lines) < 3:
            return None
        parts = lines[2].split()
        start, end = int(parts[0], 16), int(parts[1], 16)
        if start == 0 and end == 0:
            return None
        return (end - start + 1) / (1024**3)
    except (OSError, ValueError, IndexError):
        return None


def _driver_name(dev: Path) -> Optional[str]:
    link = dev / "driver"
    if not link.exists():
        return None
    try:
        return Path(os.readlink(link)).name
    except OSError:
        return None


def scan_sysfs(
    vendor: str = DEFAULT_VENDOR,
    device: str = DEFAULT_DEVICE,
) -> List[PhiDevice]:
    base = Path("/sys/bus/pci/devices")
    if not base.is_dir():
        return []

    found: List[PhiDevice] = []
    for dev in sorted(base.iterdir()):
        v = _read(dev / "vendor")
        d = _read(dev / "device")
        if v != vendor or d != device:
            continue

        sub = _read(dev / "subsystem_device") or ""
        rev = _read(dev / "revision") or ""
        cls = _read(dev / "class") or ""
        en_s = _read(dev / "enable") or "0"
        try:
            enable = int(en_s)
        except ValueError:
            enable = 0
        numa_s = _read(dev / "numa_node") or "-1"
        try:
            numa = int(numa_s)
        except ValueError:
            numa = -1

        addr = dev.name  # 0000:8b:00.0
        driver = _driver_name(dev)
        bar2 = _bar2_gib(dev / "resource")
        speed = _read(dev / "current_link_speed") or "?"
        width = _read(dev / "current_link_width") or "?"

        # "online" for KNL PCIe means driver bound + enabled; until Phase 1, False
        online = enable == 1 and driver is not None

        notes = []
        if sub and sub != DEFAULT_SUBSYS:
            notes.append(f"subsys {sub} != {DEFAULT_SUBSYS}")
        if not online:
            notes.append(
                "not enabled/no driver — need MPSS4 mic_x200 (not MPSS3)"
            )
        if bar2 and bar2 >= 16:
            notes.append(f"large BAR2 ~{bar2:.0f} GiB")
        if driver in (None, "") or driver == "none":
            notes.append("bind target: mic_x200 + mic_x200_dma")

        found.append(
            PhiDevice(
                name=f"phi{len(found)}",
                kind="phi_knl",
                model="7220P",
                online=online,
                pcie_addr=addr,
                vendor=v or "",
                device=d or "",
                subsystem_device=sub,
                revision=rev,
                class_code=cls,
                enable=enable,
                driver=driver,
                link_speed=speed,
                link_width=width,
                numa_node=numa,
                bar2_gib=bar2,
                notes="; ".join(notes),
            )
        )
    return found


def main() -> int:
    ap = argparse.ArgumentParser(description="Discover Xeon Phi 7220P (KNL)")
    ap.add_argument("--json", action="store_true", help="JSON output")
    ap.add_argument(
        "--expect-bdf",
        default=DEFAULT_BDF,
        help="Expected BDF for exit code check (default 0000:8b:00.0)",
    )
    args = ap.parse_args()

    devices = scan_sysfs()
    if args.json:
        print(json.dumps([asdict(d) for d in devices], indent=2))
    else:
        if not devices:
            print("No Phi 7220P (8086:2260) found on PCI bus.")
            return 1
        for d in devices:
            print(
                f"{d.name}: {d.model} {d.pcie_addr} "
                f"{d.vendor}:{d.device} enable={d.enable} "
                f"driver={d.driver or 'none'} "
                f"link={d.link_speed}x{d.link_width} "
                f"online={d.online}"
            )
            if d.notes:
                print(f"  notes: {d.notes}")

    if not devices:
        return 1
    # soft-check expected BDF exists
    addrs = {d.pcie_addr for d in devices}
    if args.expect_bdf not in addrs and args.expect_bdf.replace("0000:", "") not in {
        a.replace("0000:", "") for a in addrs
    }:
        # still success if any 2260 found
        print(
            f"warning: expected BDF {args.expect_bdf} not in {sorted(addrs)}",
            file=sys.stderr,
        )
    return 0


if __name__ == "__main__":
    sys.exit(main())
