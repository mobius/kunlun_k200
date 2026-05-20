#!/usr/bin/env python3
"""
End-to-end test pipeline:
  1. Read clean image (PNG/JPG)
  2. Add Gaussian noise
  3. Save noisy as PPM
  4. Run xpu_denoise (or just verify the noise)
  5. (Optional) Compare denoised output

Usage:
  python test_pipeline.py clean.png --noise 25
"""

import argparse
import subprocess
import numpy as np
import sys

def img_to_ppm(path, out_path):
    """Convert PNG/JPG to PPM using numpy (no PIL dependency)."""
    try:
        from PIL import Image
        img = Image.open(path).convert('RGB')
        arr = np.array(img)
        h, w = arr.shape[:2]
        with open(out_path, 'wb') as f:
            f.write(f'P6\n{w} {h}\n255\n'.encode())
            f.write(arr.tobytes())
        return w, h
    except ImportError:
        # Fallback: try ffmpeg
        import os
        os.system(f'ffmpeg -y -i {path} -f ppm - 2>/dev/null | tail -c +16 > {out_path}')
        # Parse size from header
        with open(out_path, 'rb') as f:
            header = f.readline().decode()
            dims = f.readline().decode()
            maxv = f.readline().decode()
            w, h = map(int, dims.split())
        return w, h

def add_noise_ppm(in_path, out_path, noise_std):
    """Add Gaussian noise to PPM image."""
    with open(in_path, 'rb') as f:
        magic = f.readline()
        dims = f.readline().decode().strip()
        maxv = f.readline().decode().strip()
        w, h = map(int, dims.split())
        data = f.read()

    arr = np.frombuffer(data, dtype=np.uint8).reshape(h, w, 3)
    noise = np.random.randn(h, w, 3).astype(np.float32) * noise_std
    noisy = np.clip(arr.astype(np.float32) + noise, 0, 255).astype(np.uint8)

    with open(out_path, 'wb') as f:
        f.write(magic)
        f.write(f'{w} {h}\n'.encode())
        f.write(maxv.encode())
        f.write(noisy.tobytes())
    print(f"Added noise std={noise_std}: {in_path} → {out_path}")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('input', help='Clean image (PNG/JPG)')
    parser.add_argument('--noise', type=float, default=25, help='Noise std')
    parser.add_argument('--denoise', default=None, help='xpu_denoise binary path')
    parser.add_argument('--dev', type=int, default=2, help='XPU device id')
    parser.add_argument('--weights', default='rand', help='Weights (rand or .bin)')
    args = parser.parse_args()

    # Convert to PPM
    print(f"Converting {args.input} to PPM...")
    w, h = img_to_ppm(args.input, '/tmp/clean.ppm')

    # Add noise
    add_noise_ppm('/tmp/clean.ppm', '/tmp/noisy.ppm', args.noise)

    if args.denoise:
        # Run XPU denoise
        cmd = [args.denoise, str(args.dev), args.weights, '/tmp/noisy.ppm', '/tmp/denoised.ppm']
        print(f"Running: {' '.join(cmd)}")
        result = subprocess.run(cmd, capture_output=True, text=True)
        print(result.stdout)
        if result.returncode != 0:
            print(f"Error: {result.stderr}")
            sys.exit(1)
        print("Done. Output: /tmp/denoised.ppm")
    else:
        print("Skipping denoise (no --denoise path).")
        print("Files: /tmp/clean.ppm  /tmp/noisy.ppm")

if __name__ == '__main__':
    main()
