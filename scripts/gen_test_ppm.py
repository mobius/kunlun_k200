#!/usr/bin/env python3
"""Generate a test PPM (optionally noisy) for C2 denoise — stdlib only."""
from __future__ import annotations

import argparse
import math
import random
from pathlib import Path


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--out", default="/tmp/case_c2_noisy.ppm")
    ap.add_argument("--w", type=int, default=512)
    ap.add_argument("--h", type=int, default=512)
    ap.add_argument("--noise", type=float, default=25.0)
    ap.add_argument("--seed", type=int, default=0)
    args = ap.parse_args()

    random.seed(args.seed)
    buf = bytearray(args.w * args.h * 3)
    i = 0
    for y in range(args.h):
        for x in range(args.w):
            r = 128 + 80 * math.sin(x / 40.0)
            g = 128 + 80 * math.cos(y / 35.0)
            b = 128 + 40 * math.sin((x + y) / 50.0)
            for c, base in enumerate((r, g, b)):
                v = base + random.gauss(0, args.noise)
                if v < 0:
                    v = 0
                if v > 255:
                    v = 255
                buf[i + c] = int(v)
            i += 3

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("wb") as f:
        f.write(f"P6\n{args.w} {args.h}\n255\n".encode())
        f.write(buf)
    print(f"wrote {out} {args.w}x{args.h} noise={args.noise}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
