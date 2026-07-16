#!/usr/bin/env python3
"""C1/S8: Paddle Inference ResNet-50 benchmark with batch sweep + 64B-aligned input.

Usage (host with paddlepaddle-xpu, or inside paddle-xpu container):
  python3 scripts/paddle_infer_benchmark.py --model paddle_models --batch 1,8,32
"""
from __future__ import annotations

import argparse
import os
import time
from pathlib import Path

import numpy as np


def find_model(model_dir: Path) -> tuple[str, str]:
    candidates = [
        (model_dir / "inference.pdmodel", model_dir / "inference.pdiparams"),
        (
            model_dir / "ResNet50_infer" / "inference.pdmodel",
            model_dir / "ResNet50_infer" / "inference.pdiparams",
        ),
    ]
    for m, p in candidates:
        if m.is_file() and p.is_file():
            return str(m), str(p)
    raise FileNotFoundError(f"no inference model under {model_dir}")


_ALIGN_KEEP: list = []


def aligned_nchw(batch: int, c: int, h: int, w: int, align: int = 64) -> np.ndarray:
    """Allocate float32 NCHW buffer with data pointer aligned to `align` bytes."""
    n = batch * c * h * w
    raw = np.empty(n + align // 4, dtype=np.float32)
    base = raw.ctypes.data
    off = (align - (base % align)) % align
    start = off // 4
    view = raw[start : start + n]
    assert view.ctypes.data % align == 0, "alignment failed"
    view = view.reshape(batch, c, h, w)
    rng = np.random.default_rng(0)
    view[:] = rng.standard_normal(view.shape, dtype=np.float32)
    _ALIGN_KEEP.append(raw)  # prevent GC of backing store
    return view


def bench(
    model_dir: Path, batches: list[int], runs: int, warmup: int, device: int
) -> list[dict]:
    import paddle.inference as pi

    os.environ["FLAGS_selected_xpus"] = str(device)
    pdmodel, pdiparams = find_model(model_dir)
    results = []

    for batch in batches:
        cfg = pi.Config(pdmodel, pdiparams)
        # paddle 2.6+: prefer parameterless enable_xpu when available
        try:
            cfg.enable_xpu()
        except TypeError:
            cfg.enable_xpu(100)
        cfg.switch_ir_optim(True)
        pred = pi.create_predictor(cfg)
        name = pred.get_input_names()[0]
        handle = pred.get_input_handle(name)
        data = aligned_nchw(batch, 3, 224, 224, 64)
        handle.reshape([batch, 3, 224, 224])
        handle.copy_from_cpu(data)

        for _ in range(warmup):
            pred.run()

        t0 = time.perf_counter()
        for _ in range(runs):
            handle.copy_from_cpu(data)
            pred.run()
        t1 = time.perf_counter()

        thr = batch * runs / (t1 - t0)
        lat = (t1 - t0) / runs * 1000.0
        row = {"batch": batch, "img_s": thr, "ms": lat}
        results.append(row)
        print(
            f"batch={batch:3d}  {thr:8.1f} img/s  {lat:7.2f} ms/batch  "
            f"(input_ptr_align64={data.ctypes.data % 64 == 0})"
        )

    return results


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--model", default="paddle_models")
    ap.add_argument("--batch", default="1,8,32", help="comma-separated batch sizes")
    ap.add_argument("--runs", type=int, default=50)
    ap.add_argument("--warmup", type=int, default=10)
    ap.add_argument("--device", type=int, default=0)
    ap.add_argument("--out", default="results/s8_paddle_resnet50.txt")
    args = ap.parse_args()

    batches = [int(x) for x in args.batch.split(",") if x.strip()]
    print("=== C1/S8 Paddle Inference ResNet-50 ===")
    print(
        f"model={args.model} runs={args.runs} warmup={args.warmup} "
        f"device={args.device} input_align=64B"
    )
    rows = bench(Path(args.model), batches, args.runs, args.warmup, args.device)

    out = Path(args.out)
    out.parent.mkdir(parents=True, exist_ok=True)
    with out.open("w") as f:
        f.write(f"C1 ResNet-50  {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
        f.write("input_buffer: 64-byte aligned float32 NCHW\n")
        for r in rows:
            f.write(f"batch={r['batch']}  {r['img_s']:.1f} img/s  {r['ms']:.2f} ms\n")
    print(f"wrote {out}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
