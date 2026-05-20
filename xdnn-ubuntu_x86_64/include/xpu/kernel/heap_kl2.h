#ifndef __XPU_KERNEL_HEAP_KL2_H
#define __XPU_KERNEL_HEAP_KL2_H

template <typename TK, typename TV>
static __device__ inline void sm_swap_kv(_shared_ptr_ TK* k0, _shared_ptr_ TV* v0,
        _shared_ptr_ TK* k1, _shared_ptr_ TV* v1) {
    TK tmpk = *k0;
    TV tmpv = *v0;
    *k0 = *k1;
    *v0 = *v1;
    *k1 = tmpk;
    *v1 = tmpv;
}

template<typename TK, typename TV>
static __device__ inline void update_sm_min_heap(_shared_ptr_ TK* heap_key,
        _shared_ptr_ TV* heap_value, int idx, int heap_capacity) {
    while (idx < heap_capacity) {
        int child_l = idx * 2 + 1;
        int child_r = idx * 2 + 2;
        int child_min = child_l;
        if (child_r >= heap_capacity) {
            if (child_l >= heap_capacity) { // idx is leaf node, shift finished
                break;
            } else {// if child_r does not exist while child_l does, choose child_l
                child_min = child_l;
            }
        } else {// both child L & R exists
            child_min = child_l + (heap_key[child_l] > heap_key[child_r]);
        }
        if (heap_key[idx] <= heap_key[child_min]) {
            break;
        }
        sm_swap_kv(&heap_key[idx], &heap_value[idx], &heap_key[child_min], &heap_value[child_min]);
        idx = child_min;
    }
}

template<typename TK, typename TV>
static __device__ inline void make_sm_min_heap(
        _shared_ptr_ TK* heap_key, _shared_ptr_  TV* heap_value, int size) {
    for (int i = size / 2 - 1; i >= 0; i--) {
        update_sm_min_heap(heap_key, heap_value, i, size);
    }
}

template<typename TK, typename TV>
static __device__ inline void sort_sm_min_heap(
        _shared_ptr_ TK* heap_key, _shared_ptr_ TV* heap_value, int heap_capacity) {
    for (int i = heap_capacity - 1; i > 0; i--) {
        sm_swap_kv(&heap_key[0], &heap_value[0], &heap_key[i], &heap_value[i]);
        update_sm_min_heap(heap_key, heap_value, 0, i);
    }
}

template<typename TK, typename TV>
static __device__ inline void update_sm_max_heap(_shared_ptr_ TK* heap_key,
        _shared_ptr_ TV* heap_value, int idx, int heap_capacity) {
    while (idx < heap_capacity) {
        int child_l = idx * 2 + 1;
        int child_r = idx * 2 + 2;
        int child_max = child_l;
        if (child_r >= heap_capacity) {
            if (child_l >= heap_capacity) { // idx is leaf node, shift finished
                break;
            } else {// if child_r does not exist while child_l does, choose child_l
                child_max = child_l;
            }
        } else {// both child L & R exists
            child_max = child_l + (heap_key[child_l] < heap_key[child_r]);
        }
        if (heap_key[idx] >= heap_key[child_max]) {
            break;
        }
        sm_swap_kv(&heap_key[idx], &heap_value[idx], &heap_key[child_max], &heap_value[child_max]);
        idx = child_max;
    }
}

template<typename TK, typename TV>
static __device__ inline void make_sm_max_heap(
        _shared_ptr_ TK* heap_key, _shared_ptr_ TV* heap_value, int size) {
    for (int i = size / 2 - 1; i >= 0; i--) {
        update_sm_max_heap(heap_key, heap_value, i, size);
    }
}

template<typename TK, typename TV>
static __device__ inline void sort_sm_max_heap(_shared_ptr_ TK* heap_key,
        _shared_ptr_ TV* heap_value, int heap_capacity) {
    for (int i = heap_capacity - 1; i > 0; i--) {
        sm_swap_kv(&heap_key[0], &heap_value[0], &heap_key[i], &heap_value[i]);
        update_sm_max_heap(heap_key, heap_value, 0, i);
    }
}

template <typename TK, typename TV>
static __device__ inline void lm_swap_kv(TK* k0, TV* v0,
        TK* k1, TV* v1) {
    TK tmpk = *k0;
    TV tmpv = *v0;
    *k0 = *k1;
    *v0 = *v1;
    *k1 = tmpk;
    *v1 = tmpv;
}

template <typename TK, typename TV>
static __device__ inline void update_lm_min_heap(TK* heap_key, TV* heap_value, int idx, int heap_capacity) {
    while (idx < heap_capacity) {
        int child_l = idx * 2 + 1;
        int child_r = idx * 2 + 2;
        int child_min = child_l;
        if (child_r >= heap_capacity) {
            if (child_l >= heap_capacity) { // idx is leaf node, shift finished
                break;
            } else {// if child_r does not exist while child_l does, choose child_l
                child_min = child_l;
            }
        } else {// both child L & R exists
            child_min = child_l + (heap_key[child_l] > heap_key[child_r]);
        }
        if (heap_key[idx] <= heap_key[child_min]) {
            break;
        }
        lm_swap_kv(&heap_key[idx], &heap_value[idx], &heap_key[child_min], &heap_value[child_min]);
        idx = child_min;
    }
}

template<typename TK, typename TV>
static __device__ inline void make_lm_min_heap(
        TK* heap_key, TV* heap_value, int size) {
    for (int i = size / 2 - 1; i >= 0; i--) {
        update_lm_min_heap(heap_key, heap_value, i, size);
    }
}

template<typename TK, typename TV>
static __device__ inline void sort_lm_min_heap(TK* heap_key, TV* heap_value, int heap_capacity) {
    for (int i = heap_capacity - 1; i > 0; i--) {
        lm_swap_kv(&heap_key[0], &heap_value[0], &heap_key[i], &heap_value[i]);
        update_lm_min_heap(heap_key, heap_value, 0, i);
    }
}

template<typename TK, typename TV>
static __device__ inline void update_lm_max_heap(TK* heap_key, TV* heap_value, int idx, int heap_capacity) {
    while (idx < heap_capacity) {
        int child_l = idx * 2 + 1;
        int child_r = idx * 2 + 2;
        int child_max = child_l;
        if (child_r >= heap_capacity) {
            if (child_l >= heap_capacity) { // idx is leaf node, shift finished
                break;
            } else {// if child_r does not exist while child_l does, choose child_l
                child_max = child_l;
            }
        } else {// both child L & R exists
            child_max = child_l + (heap_key[child_l] < heap_key[child_r]);
        }
        if (heap_key[idx] >= heap_key[child_max]) {
            break;
        }
        lm_swap_kv(&heap_key[idx], &heap_value[idx], &heap_key[child_max], &heap_value[child_max]);
        idx = child_max;
    }
}

template<typename TK, typename TV>
static __device__ inline void make_lm_max_heap(
        TK* heap_key, TV* heap_value, int size) {
    for (int i = size / 2 - 1; i >= 0; i--) {
        update_lm_max_heap(heap_key, heap_value, i, size);
    }
}

template<typename TK, typename TV>
static __device__ inline void sort_lm_max_heap(TK* heap_key, TV* heap_value, int heap_capacity) {
    for (int i = heap_capacity - 1; i > 0; i--) {
        lm_swap_kv(&heap_key[0], &heap_value[0], &heap_key[i], &heap_value[i]);
        update_lm_max_heap(heap_key, heap_value, 0, i);
    }
}

#endif
