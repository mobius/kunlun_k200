#!/usr/bin/env python3
"""PSNR between two P6 PPMs (stdlib)."""
from __future__ import annotations

import argparse
import math
import sys
from pathlib import Path


def read_ppm(path: Path) -> tuple[int, int, bytes]:
    with path.open("rb") as f:
        magic = f.readline().strip()
        if magic != b"P6":
            raise ValueError(f"not P6: {path}")
        # skip comments
        line = f.readline()
        while line.startswith(b"#"):
            line = f.readline()
        w, h = map(int, line.split())
        maxv = int(f.readline())
        if maxv != 255:
            raise ValueError("maxval must be 255")
        data = f.read()
    return w, h, data


def psnr(a: bytes, b: bytes) -> float:
    if len(a) != len(b) or not a:
        raise ValueError("size mismatch")
    sse = 0.0
    for x, y in zip(a, b):
        d = float(x) - float(y)
        sse += d * d
    mse = sse / len(a)
    if mse <= 1e-12:
        return float("inf")
    return 10.0 * math.log10((255.0 * 255.0) / mse)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("ref")
    ap.add_argument("test")
    args = ap.parse_args()
    wr, hr, ref = read_ppm(Path(args.ref))
    wt, ht, tst = read_ppm(Path(args.test))
    if (wr, hr) != (wt, ht):
        print(f"dim mismatch {wr}x{hr} vs {wt}x{ht}", file=sys.stderr)
        return 1
    v = psnr(ref, tst)
    print(f"PSNR: {v:.3f} dB  ({args.test} vs {args.ref}, {wr}x{hr})")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
