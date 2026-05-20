#include "xpu/kernel/cluster_header.h"
#include "xpu/kernel/math.h"

template <typename T>
static __device__ void swap(T* a, T* b) {
    T tmp = *a;
    *a = *b;
    *b = tmp;
}

static __device__ void min_heapify(float* heap_key, int* heap_value, int idx, int heap_capacity) {
    while (idx < heap_capacity) {
        int child_l = idx * 2 + 1;
        int child_r = idx * 2 + 2;
        int child_min = child_l;
        if (child_r >= heap_capacity) {
            if (child_l >= heap_capacity) {
                // idx is leaf node, shift finished
                break;
            } else {
                // if child_r does not exist while child_l does, choose child_l
                child_min = child_l;
            }
        } else {
            // both child L & R exists
            // child_min = (heap_key[child_l] <= heap_key[child_r]) ? child_l : child_r;
            // int v = (heap_key[child_l] > heap_key[child_r]) ? 1: 0;
            int v = 0;
            __asm__("setgt_s %0, %1, %2":"=&r"(v):"r"(heap_key[child_l]), "r"(heap_key[child_r]));
            child_min = v + child_l;
        }
        if (heap_key[idx] <= heap_key[child_min]) {
            break;
        }
        swap(heap_key + idx, heap_key + child_min);
        swap(heap_value + idx, heap_value + child_min);
        idx = child_min;
    }
}

static __device__ void max_heapify(float* heap_key, int* heap_value, int idx, int heap_capacity) {
    while (idx < heap_capacity) {
        int child_l = idx * 2 + 1;
        int child_r = idx * 2 + 2;
        int child_max = child_l;
        if (child_r >= heap_capacity) {
            if (child_l >= heap_capacity) {
                // idx is leaf node, shift finished
                break;
            } else {
                // if child_r does not exist while child_l does, choose child_l
                child_max = child_l;
            }
        } else {
            // both child L & R exists
            child_max = (heap_key[child_l] >= heap_key[child_r]) ? child_l : child_r;
        }
        if (heap_key[idx] >= heap_key[child_max]) {
            break;
        }
        swap(heap_key + idx, heap_key + child_max);
        swap(heap_value + idx, heap_value + child_max);
        idx = child_max;
    }
}

static __device__ void min_heap_build(float* heap_key, int* heap_value, int size) {
    for (int i = size / 2 - 1; i >= 0; i--) {
        min_heapify(heap_key, heap_value, i, size);
    }
}

static __device__ void max_heap_build(float* heap_key, int* heap_value, int size) {
    for (int i = size / 2 - 1; i >= 0; i--) {
        max_heapify(heap_key, heap_value, i, size);
    }
}

static __device__ void min_heap_sort(float* heap_key, int* heap_value, int heap_capacity) {
    for (int i = heap_capacity - 1; i > 0; i--) {
        swap(heap_key, heap_key + i);
        swap(heap_value, heap_value + i);
        min_heapify(heap_key, heap_value, 0, i);
    }
}

static __device__ void max_heap_sort(float* heap_key, int* heap_value, int heap_capacity) {
    for (int i = heap_capacity - 1; i > 0; i--) {
        swap(heap_key, heap_key + i);
        swap(heap_value, heap_value + i);
        max_heapify(heap_key, heap_value, 0, i);
    }
}
