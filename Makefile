CXX = g++
CXXFLAGS = -std=c++17 -O2 -I/usr/local/xpu-4.33.0/include -I/mnt/storage/test_xpu/xdnn-ubuntu_x86_64/include
LDFLAGS = -L/usr/local/xpu-4.33.0/lib64 -L/mnt/storage/test_xpu/xdnn-ubuntu_x86_64/so -lxpuapi -lxpurt -Wl,-rpath,/usr/local/xpu-4.33.0/lib64 -Wl,-rpath,/mnt/storage/test_xpu/xdnn-ubuntu_x86_64/so

TARGET = xpu_perf_test
DENOISE = xpu_denoise

all: $(TARGET) $(DENOISE)

$(TARGET): xpu_perf_test.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

$(DENOISE): xpu_denoise.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGET) $(DENOISE)

run: $(TARGET)
	./$(TARGET) 0

.PHONY: all clean run
