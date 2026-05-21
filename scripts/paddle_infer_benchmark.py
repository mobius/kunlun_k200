import paddle.inference as paddle_infer
import numpy as np
import time
import os

os.environ['FLAGS_selected_xpus'] = '0'

config = paddle_infer.Config("/mnt/storage/test_xpu/paddle_models/inference.pdmodel",
                             "/mnt/storage/test_xpu/paddle_models/inference.pdiparams")
config.enable_xpu(100)
config.switch_ir_optim(True)

predictor = paddle_infer.create_predictor(config)

input_names = predictor.get_input_names()
input_tensor = predictor.get_input_handle(input_names[0])

batch_size = 1
img_h = 224
img_w = 224
warmup = 10
runs = 100

data = np.random.randn(batch_size, 3, img_h, img_w).astype("float32")
input_tensor.reshape([batch_size, 3, img_h, img_w])
input_tensor.copy_from_cpu(data)

print(f"=== Paddle Inference ResNet-50 Benchmark ===")
print(f"Batch size: {batch_size}, Input: {img_h}x{img_w}")
print(f"Warmup: {warmup}, Runs: {runs}")

for _ in range(warmup):
    predictor.run()

start = time.perf_counter()
for _ in range(runs):
    predictor.run()
end = time.perf_counter()

elapsed = end - start
latency_ms = elapsed / runs * 1000
throughput = batch_size * runs / elapsed

print(f"\nTotal time: {elapsed:.3f}s")
print(f"Avg latency: {latency_ms:.2f} ms/batch")
print(f"Throughput: {throughput:.1f} images/sec")

with open('/mnt/storage/test_xpu/results/paddle_resnet50_infer.txt', 'w') as f:
    f.write(f"ResNet-50 Paddle Inference XPU Benchmark\n")
    f.write(f"Date: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
    f.write(f"Batch size: {batch_size}\n")
    f.write(f"Input: {img_h}x{img_w}\n")
    f.write(f"Runs: {runs}\n")
    f.write(f"Avg latency: {latency_ms:.2f} ms/batch\n")
    f.write(f"Throughput: {throughput:.1f} images/sec\n")

print("Results saved to /mnt/storage/test_xpu/results/paddle_resnet50_infer.txt")
