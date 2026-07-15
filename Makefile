ROOT = /mnt/storage/test_xpu
CXX = g++
CXXFLAGS = -std=c++17 -O2 -I/usr/local/xpu-4.33.0/include -I$(ROOT)/xdnn-ubuntu_x86_64/include
LDFLAGS = -L/usr/local/xpu-4.33.0/lib64 -L$(ROOT)/xdnn-ubuntu_x86_64/so -lxpuapi -lxpurt \
          -Wl,-rpath,/usr/local/xpu-4.33.0/lib64 -Wl,-rpath,$(ROOT)/xdnn-ubuntu_x86_64/so

BENCHMARKS = benchmarks/xpu_perf_test benchmarks/xpu_denoise benchmarks/xpu_int8_probe \
	benchmarks/xpu_app_pipeline benchmarks/xpu_pipeline_p2p
TESTS = tests/test_p2p tests/test_p2p_verify tests/test_host_alloc tests/test_pageable_verify

all: $(BENCHMARKS) $(TESTS)

benchmarks/xpu_perf_test: benchmarks/xpu_perf_test.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

benchmarks/xpu_denoise: benchmarks/xpu_denoise.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

benchmarks/xpu_int8_probe: benchmarks/xpu_int8_probe.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

benchmarks/xpu_app_pipeline: benchmarks/xpu_app_pipeline.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

benchmarks/xpu_pipeline_p2p: benchmarks/xpu_pipeline_p2p.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

tests/test_p2p: tests/test_p2p.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

tests/test_p2p_verify: tests/test_p2p_verify.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

tests/test_host_alloc: tests/test_host_alloc.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

tests/test_pageable_verify: tests/test_pageable_verify.cpp
	$(CXX) $(CXXFLAGS) -o $@ $< $(LDFLAGS)

driver:
	$(MAKE) -C kunlun-driver modules

driver-install: driver
	sudo KL1_P2P_STUB=$${KL1_P2P_STUB:-0} \
		KL1_DMA_DIRECT=$${KL1_DMA_DIRECT:-1} \
		KL1_BOUNCE_PIPE=$${KL1_BOUNCE_PIPE:-1} \
		scripts/install_driver.sh

clean:
	rm -f $(BENCHMARKS) $(TESTS)
	$(MAKE) -C kunlun-driver clean 2>/dev/null || true

run: benchmarks/xpu_perf_test
	./benchmarks/xpu_perf_test 0

# S7 gate: correctness + S4/S5/S6 bandwidth floors
regression: tests/test_p2p_verify tests/test_host_alloc tests/test_pageable_verify \
		tests/test_p2p benchmarks/xpu_perf_test
	scripts/run_driver_regression.sh

# Real-world cases C1+C2+C3 (see docs/plan/20260715-real-world-case-plan.md)
cases: benchmarks/xpu_denoise benchmarks/xpu_pipeline_p2p benchmarks/xpu_app_pipeline
	scripts/run_real_cases.sh

.PHONY: all clean run driver driver-install regression cases
