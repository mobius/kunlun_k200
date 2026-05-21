#!/usr/bin/env python3
"""Generate synthetic denoising weights (pure Python, no numpy)."""
import sys, struct, math

N_BLOCKS, MID_CH = 8, 32

def gen_conv(cout, cin, ks, scale=0.9):
    w = []
    for o in range(cout):
        for i in range(cin):
            for p in range(ks*ks):
                if i == o and cin == cout:
                    w.append(scale/(ks*ks) + (hash((o,i,p))%1000-500)/500000.0)
                else:
                    w.append((hash((o,i,p))%1000-500)/500000.0)
    return w

data = []
data.extend(gen_conv(32, 3, 3, 0.5))
for b in range(N_BLOCKS):
    data.extend(gen_conv(32, 32, 3, 0.3))
    data.extend(gen_conv(32, 32, 3, 0.3))
data.extend(gen_conv(32, 32, 3, 0.3))
data.extend(gen_conv(3, 32, 3, 0.5))
n_bias = 32 + N_BLOCKS*2*32 + 32 + 3
data.extend([0.0]*n_bias)

out = sys.argv[1] if len(sys.argv)>1 else 'xpu_denoise_synth.bin'
with open(out, 'wb') as f:
    for v in data:
        f.write(struct.pack('e', float(v)))  # IEEE 754 half-precision
print(f'Wrote {len(data)} float16 → {out}')
