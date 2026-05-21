#!/usr/bin/env python3
"""
PyTorch → XDNN weight converter for XPU-Denoise model.

Model architecture:
  conv_first: Conv2d(3, 32, 3, padding=1) + LeakyReLU
  8× ResBlock:
    conv1: Conv2d(32, 32, 3, padding=1) + LeakyReLU
    conv2: Conv2d(32, 32, 3, padding=1)
    skip: x = x + conv2_out
  conv_mid:  Conv2d(32, 32, 3, padding=1)
  global:    x = conv_first_out + conv_mid_out
  conv_last: Conv2d(32, 3, 3, padding=1)

Weight layout (all float16):
  [conv_first.weight (3*3*3*32)]
  [rb0_conv1.weight (32*3*3*32)] [rb0_conv2.weight]
  ... x8 ...
  [conv_mid.weight (32*3*3*32)]
  [conv_last.weight (32*3*3*3)]
  [conv_first.bias (32)]
  [rb0_conv1.bias (32)] [rb0_conv2.bias (32)]
  ... x8 ...
  [conv_mid.bias (32)]
  [conv_last.bias (3)]

Usage:
  python convert_weights.py model.pth output.bin
"""

import sys
import struct
import numpy as np
import torch

N_BLOCKS = 8
MID_CH = 32

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <model.pth> <output.bin>")
        sys.exit(1)

    ckpt = torch.load(sys.argv[1], map_location='cpu', weights_only=True)
    if 'state_dict' in ckpt:
        ckpt = ckpt['state_dict']

    weights = []
    biases = []

    def add_conv(name):
        w = ckpt[f'{name}.weight'].float().numpy()
        b = ckpt[f'{name}.bias'].float().numpy() if f'{name}.bias' in ckpt else np.zeros(w.shape[0])
        weights.append(w.ravel())
        biases.append(b.ravel())

    # Feature extraction
    add_conv('conv_first')

    # Residual blocks
    for i in range(N_BLOCKS):
        add_conv(f'blocks.{i}.conv1')
        add_conv(f'blocks.{i}.conv2')

    # Mid and last
    add_conv('conv_mid')
    add_conv('conv_last')

    # Concatenate: all weights then all biases
    all_w = np.concatenate(weights).astype(np.float16)
    all_b = np.concatenate(biases).astype(np.float16)
    all_data = np.concatenate([all_w, all_b])

    with open(sys.argv[2], 'wb') as f:
        f.write(all_data.tobytes())

    print(f"Converted: {len(all_w)} weights + {len(all_b)} biases → {sys.argv[2]}")
    print(f"Total: {len(all_data)} float16 values = {len(all_data)*2} bytes")

if __name__ == '__main__':
    main()
