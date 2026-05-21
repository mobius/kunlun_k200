#!/usr/bin/env python3
"""
Train XPU-Denoise model for K200 deployment.

Architecture: 8× ResBlock CNN (must match xpu_denoise.cpp)
  conv_first: Conv2d(3, 32, 3, pad=1) + LeakyReLU(0.1)
  8× ResBlock: Conv(32,32,3,pad=1)+LeakyReLU → Conv(32,32,3,pad=1) → skip_add
  conv_mid:   Conv2d(32, 32, 3, pad=1)
  global_skip: conv_first_out + conv_mid_out
  conv_last:  Conv2d(32, 3, 3, pad=1)

Usage:
  python train_denoise.py --epochs 20 --batch 8 --lr 0.001
  python train_denoise.py --data ./dataset --out model.pth
"""

import argparse
import os
import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader
from PIL import Image
import random

# ============================================================
# Model (must match xpu_denoise.cpp layout)
# ============================================================
class ResBlock(nn.Module):
    def __init__(self, ch=32):
        super().__init__()
        self.conv1 = nn.Conv2d(ch, ch, 3, padding=1)
        self.conv2 = nn.Conv2d(ch, ch, 3, padding=1)
        self.act = nn.LeakyReLU(0.1, inplace=True)

    def forward(self, x):
        r = self.act(self.conv1(x))
        r = self.conv2(r)
        return x + r

class XPUDenoise(nn.Module):
    def __init__(self, n_blocks=8, mid_ch=32):
        super().__init__()
        self.conv_first = nn.Conv2d(3, mid_ch, 3, padding=1)
        self.act_first = nn.LeakyReLU(0.1, inplace=True)
        self.blocks = nn.ModuleList([ResBlock(mid_ch) for _ in range(n_blocks)])
        self.conv_mid = nn.Conv2d(mid_ch, mid_ch, 3, padding=1)
        self.conv_last = nn.Conv2d(mid_ch, 3, 3, padding=1)

    def forward(self, x):
        f = self.act_first(self.conv_first(x))  # feature extraction
        h = f
        for blk in self.blocks:
            h = blk(h)
        h = self.conv_mid(h)
        h = f + h                                # global skip
        return self.conv_last(h)

# ============================================================
# Synthetic dataset generator (noise patches)
# ============================================================
class NoiseDataset(Dataset):
    """Generate (noisy, clean) pairs from directory of clean images."""
    def __init__(self, image_dir, patch_size=128, samples=5000, noise_std=25):
        self.patch_size = patch_size
        self.samples = samples
        self.noise_std = noise_std
        self.files = []
        if os.path.isdir(image_dir):
            self.files = [os.path.join(image_dir, f) for f in os.listdir(image_dir)
                          if f.lower().endswith(('.png', '.jpg', '.jpeg', '.bmp'))]
        if not self.files:
            print("Warning: no images found, using random synthetic patches")

    def __len__(self):
        return self.samples

    def __getitem__(self, idx):
        ps = self.patch_size
        if self.files:
            img = Image.open(random.choice(self.files)).convert('RGB')
            img = img.resize((max(ps, img.width), max(ps, img.height)))
            arr = np.array(img).astype(np.float32) / 255.0
            h, w = arr.shape[:2]
            y = random.randint(0, h - ps)
            x = random.randint(0, w - ps)
            clean = arr[y:y+ps, x:x+ps]
        else:
            # Random gradient patch
            gx = np.linspace(0, 1, ps).reshape(1, ps, 1)
            gy = np.linspace(0, 1, ps).reshape(ps, 1, 1)
            clean = 0.3 * gx + 0.3 * gy + 0.2 * np.random.rand(ps, ps, 1)
            clean = np.tile(clean, (1, 1, 3))

        noise = np.random.randn(ps, ps, 3).astype(np.float32) * (self.noise_std / 255.0)
        noisy = np.clip(clean + noise, 0, 1)
        clean = torch.from_numpy(clean).permute(2, 0, 1)  # CHW
        noisy = torch.from_numpy(noisy).permute(2, 0, 1)
        return noisy, clean

# ============================================================
# Training
# ============================================================
def train(args):
    device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')
    print(f"Device: {device}")

    model = XPUDenoise(n_blocks=8, mid_ch=32).to(device)
    n_params = sum(p.numel() for p in model.parameters())
    print(f"Model: {n_params} params ({n_params*2/1024:.1f} KB FP16)")

    dataset = NoiseDataset(args.data, patch_size=args.patch, samples=args.samples, noise_std=args.noise)
    loader = DataLoader(dataset, batch_size=args.batch, shuffle=True, num_workers=2)

    criterion = nn.L1Loss()
    optimizer = optim.Adam(model.parameters(), lr=args.lr)
    scheduler = optim.lr_scheduler.CosineAnnealingLR(optimizer, args.epochs)

    for epoch in range(args.epochs):
        total_loss = 0
        for i, (noisy, clean) in enumerate(loader):
            noisy, clean = noisy.to(device), clean.to(device)
            output = model(noisy)
            loss = criterion(output, clean)
            optimizer.zero_grad()
            loss.backward()
            optimizer.step()
            total_loss += loss.item()

            if i % 50 == 0:
                print(f"  Epoch {epoch+1}/{args.epochs} [{i}/{len(loader)}] loss={loss.item():.6f}")

        scheduler.step()
        avg_loss = total_loss / len(loader)
        print(f"Epoch {epoch+1}: avg_loss={avg_loss:.6f}, lr={scheduler.get_last_lr()[0]:.6f}")

    # Save
    save_dict = {k: v for k, v in model.state_dict().items()}
    torch.save(save_dict, args.out)
    print(f"Saved: {args.out}")

    # Print key names for verification
    print("\nState dict keys:")
    for k in sorted(save_dict.keys()):
        print(f"  {k}: {list(save_dict[k].shape)}")

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--data', default='', help='Image directory (optional)')
    parser.add_argument('--out', default='xpu_denoise.pth', help='Output checkpoint')
    parser.add_argument('--epochs', type=int, default=20)
    parser.add_argument('--batch', type=int, default=8)
    parser.add_argument('--lr', type=float, default=0.001)
    parser.add_argument('--patch', type=int, default=128)
    parser.add_argument('--samples', type=int, default=5000)
    parser.add_argument('--noise', type=float, default=25, help='Noise std (in 0-255 scale)')
    args = parser.parse_args()
    train(args)
