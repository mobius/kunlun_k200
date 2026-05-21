import paddle
import paddle.nn as nn
import time
import os

os.environ['FLAGS_selected_xpus'] = '0'
paddle.set_device('xpu:0')

print(f"Paddle version: {paddle.__version__}")
print(f"XPU device count: {paddle.device.xpu.device_count()}")

# Build ResNet-50
class Bottleneck(nn.Layer):
    expansion = 4
    def __init__(self, in_planes, planes, stride=1):
        super().__init__()
        self.conv1 = nn.Conv2D(in_planes, planes, 1, bias_attr=False)
        self.bn1 = nn.BatchNorm2D(planes)
        self.conv2 = nn.Conv2D(planes, planes, 3, stride=stride, padding=1, bias_attr=False)
        self.bn2 = nn.BatchNorm2D(planes)
        self.conv3 = nn.Conv2D(planes, planes * self.expansion, 1, bias_attr=False)
        self.bn3 = nn.BatchNorm2D(planes * self.expansion)
        self.shortcut = nn.Sequential()
        if stride != 1 or in_planes != planes * self.expansion:
            self.shortcut = nn.Sequential(
                nn.Conv2D(in_planes, planes * self.expansion, 1, stride=stride, bias_attr=False),
                nn.BatchNorm2D(planes * self.expansion)
            )
    def forward(self, x):
        out = paddle.nn.functional.relu(self.bn1(self.conv1(x)))
        out = paddle.nn.functional.relu(self.bn2(self.conv2(out)))
        out = self.bn3(self.conv3(out))
        out += self.shortcut(x)
        return paddle.nn.functional.relu(out)

class ResNet(nn.Layer):
    def __init__(self, block, num_blocks, num_classes=1000):
        super().__init__()
        self.in_planes = 64
        self.conv1 = nn.Conv2D(3, 64, 7, stride=2, padding=3, bias_attr=False)
        self.bn1 = nn.BatchNorm2D(64)
        self.maxpool = nn.MaxPool2D(3, stride=2, padding=1)
        self.layer1 = self._make_layer(block, 64, num_blocks[0], stride=1)
        self.layer2 = self._make_layer(block, 128, num_blocks[1], stride=2)
        self.layer3 = self._make_layer(block, 256, num_blocks[2], stride=2)
        self.layer4 = self._make_layer(block, 512, num_blocks[3], stride=2)
        self.avgpool = nn.AdaptiveAvgPool2D((1, 1))
        self.fc = nn.Linear(512 * block.expansion, num_classes)
    def _make_layer(self, block, planes, num_blocks, stride):
        layers = []
        layers.append(block(self.in_planes, planes, stride))
        self.in_planes = planes * block.expansion
        for _ in range(1, num_blocks):
            layers.append(block(self.in_planes, planes))
        return nn.Sequential(*layers)
    def forward(self, x):
        x = paddle.nn.functional.relu(self.bn1(self.conv1(x)))
        x = self.maxpool(x)
        x = self.layer1(x)
        x = self.layer2(x)
        x = self.layer3(x)
        x = self.layer4(x)
        x = self.avgpool(x)
        x = paddle.flatten(x, 1)
        return self.fc(x)

def ResNet50():
    return ResNet(Bottleneck, [3, 4, 6, 3])

model = ResNet50()
model.eval()

# Synthetic data
batch_size = 32
img_h = 224
img_w = 224
warmup = 10
runs = 100

x = paddle.randn([batch_size, 3, img_h, img_w], 'float32')
x = paddle.to_tensor(x, place=paddle.XPUPlace(0))

print(f"\n=== ResNet-50 Benchmark ===")
print(f"Batch size: {batch_size}, Input: {img_h}x{img_w}")
print(f"Warmup: {warmup}, Runs: {runs}")

# Warmup
for _ in range(warmup):
    with paddle.no_grad():
        _ = model(x)
paddle.device.xpu.synchronize(0)

# Measure
start = time.perf_counter()
for _ in range(runs):
    with paddle.no_grad():
        _ = model(x)
paddle.device.xpu.synchronize(0)
end = time.perf_counter()

elapsed = end - start
latency_ms = elapsed / runs * 1000
throughput = batch_size * runs / elapsed

print(f"\nTotal time: {elapsed:.3f}s")
print(f"Avg latency: {latency_ms:.2f} ms/batch")
print(f"Throughput: {throughput:.1f} images/sec")

# Save results
with open('/mnt/storage/test_xpu/results/paddle_resnet50.txt', 'w') as f:
    f.write(f"ResNet-50 XPU Benchmark\n")
    f.write(f"Date: {time.strftime('%Y-%m-%d %H:%M:%S')}\n")
    f.write(f"Paddle version: {paddle.__version__}\n")
    f.write(f"XPU device: 0\n")
    f.write(f"Batch size: {batch_size}\n")
    f.write(f"Input: {img_h}x{img_w}\n")
    f.write(f"Runs: {runs}\n")
    f.write(f"Avg latency: {latency_ms:.2f} ms/batch\n")
    f.write(f"Throughput: {throughput:.1f} images/sec\n")

print("\nResults saved to /mnt/storage/test_xpu/results/paddle_resnet50.txt")
