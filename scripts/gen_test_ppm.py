#!/usr/bin/env python3
"""Generate clean + optional noisy PPM for C2 denoise — stdlib only."""
from __future__ import annotations

import argparse
import math
import random
from pathlib import Path


def render(w: int, h: int, noise: float, seed: int) -> tuple[bytearray, bytearray]:
    random.seed(seed)
    clean = bytearray(w * h * 3)
    noisy = bytearray(w * h * 3)
    i = 0
    for y in range(h):
        for x in range(w):
            r = 128 + 80 * math.sin(x / 40.0)
            g = 128 + 80 * math.cos(y / 35.0)
            b = 128 + 40 * math.sin((x + y) / 50.0)
            for c, base in enumerate((r, g, b)):
                cv = max(0, min(255, int(base)))
                clean[i + c] = cv
                if noise > 0:
                    nv = base + random.gauss(0, noise)
                    noisy[i + c] = max(0, min(255, int(nv)))
                else:
                    noisy[i + c] = cv
            i += 3
    return clean, noisy


def write_ppm(path: Path, w: int, h: int, buf: bytearray) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("wb") as f:
        f.write(f"P6\n{w} {h}\n255\n".encode())
        f.write(buf)


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("-o", "--out", default="/tmp/case_c2_noisy.ppm", help="noisy output")
    ap.add_argument("--clean", default="", help="optional clean PPM path")
    ap.add_argument("--w", type=int, default=512)
    ap.add_argument("--h", type=int, default=512)
    ap.add_argument("--noise", type=float, default=25.0)
    ap.add_argument("--seed", type=int, default=0)
    args = ap.parse_args()

    clean, noisy = render(args.w, args.h, args.noise, args.seed)
    write_ppm(Path(args.out), args.w, args.h, noisy)
    print(f"wrote {args.out} {args.w}x{args.h} noise={args.noise}")
    if args.clean:
        write_ppm(Path(args.clean), args.w, args.h, clean)
        print(f"wrote {args.clean} (clean)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
