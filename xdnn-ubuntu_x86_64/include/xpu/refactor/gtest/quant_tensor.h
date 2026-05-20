#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CORE_QUANT_TENSOR_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CORE_QUANT_TENSOR_H

#include "xpu/refactor/core/quant.h"
#include "xpu/refactor/gtest/tensor.h"
#include <iostream>

namespace baidu {
namespace xpu {
namespace api {

class QuantTensor {
public:
    inline QuantTensor(const QuantTensor& other) : _val(other._val), _max(other._max) {
    }
    inline QuantTensor& operator=(const QuantTensor& other) {
        _val = other._val;
        _max = other._max;
        return *this;
    }
    inline QuantTensor& operator=(QuantTensor&& other) {
        _val = other._val;
        _max = other._max;
        return *this;
    }
    // bool valid() const {return _val.valid() && _max.valid();}
    inline QuantTensor to(Device in_dev) const {
        QuantTensor ret(*this);
        ret._val = _val.to(in_dev);
        ret._max = _max.to(in_dev);
        return ret;
    }
    inline void to_l3(Device in_dev) {
        this->_val.to_l3(in_dev);
        this->_max.to_l3(in_dev);
        return;
    }
    inline int load_from_npy(InOrOutKind kind) {
        return (this->_val.load_from_npy(kind) || this->_max.load_from_npy(kind));
    }
    inline int load_from_npy(DeviceType dev, InOrOutKind kind) {
        return (this->_val.load_from_npy(dev, kind) || this->_max.load_from_npy(dev, kind));
    }
    inline int save_to_npy(InOrOutKind kind) {
        return (this->_val.save_to_npy(kind) || this->_max.save_to_npy(kind));
    }
    Tensor& val() {
        return _val;
    }
    Tensor& max() {
        return _max;
    }
private:
    inline QuantTensor(): _val(tensor(1.0f)), _max(tensor(1.0f)) {}
    Tensor _val;
    Tensor _max;

    friend QuantTensor Quant(const Tensor& t, Dtype dst_type);
    friend int max_allclose(const QuantTensor& t0, const QuantTensor& t1, float rtol, float atol);
};

// Creation
inline QuantTensor Quant(const Tensor& t, Dtype dst_type) {
    // t.dtype() must be kFLOAT32 or kFLOAT16
    // dst_type  must be kFLOAT32 or kFLOAT16 or kINT16 or kINT8 or kINT32 or kINT4
    QuantTensor ret;
    Tensor cpu_t = t.to(kCPU).astype(kFLOAT32);
    bool t_is_ok = (t.dtype() == kFLOAT32 || t.dtype() == kFLOAT16);
    bool dst_is_ok = (dst_type == kFLOAT32 || dst_type == kFLOAT16 || dst_type == kBFLOAT16 || dst_type == kINT32 || dst_type == kINT16 
        || dst_type == kINT8 || dst_type == kINT4);
    if (!t_is_ok || !dst_is_ok) {
        std::vector<float> tmp_vec = {};
        // set to a invalid tensor
        ret._val = tensor(tmp_vec, get_val_tensor_in_or_out_kind(t.in_or_out_kind()), t.tensor_name());
    } else {
        float cpu_max = quant_findmax<float>(cpu_t.data<float>(), cpu_t.numel(), nullptr);
        std::vector<float> tmp_vec(16, 0);
        tmp_vec[0] = cpu_max;
        ret._max = tensor(tmp_vec, get_max_tensor_in_or_out_kind(t.in_or_out_kind()), t.tensor_name() + "_max");
        ret._val = cpu_t.astype(dst_type);
        ret._val.set_in_or_out_kind(get_val_tensor_in_or_out_kind(t.in_or_out_kind()));
        ret._val.set_tensor_name(t.tensor_name());
        if (dst_type == kINT32) {
            std::vector<int> tmp = quant_vector<float, int>(cpu_t.data<float>(), cpu_t.numel(), cpu_max);
            std::memcpy(ret._val.data<int>(), tmp.data(), tmp.size() * sizeof(int));
        }
        if (dst_type == kINT16) {
            std::vector<int16_t> tmp = quant_vector<float, int16_t>(cpu_t.data<float>(), cpu_t.numel(), cpu_max);
            std::memcpy(ret._val.data<int16_t>(), tmp.data(), tmp.size() * sizeof(int16_t));
        }
        if (dst_type == kINT8) {
            std::vector<int8_t> tmp = quant_vector<float, int8_t>(cpu_t.data<float>(), cpu_t.numel(), cpu_max);
            std::memcpy(ret._val.data<int8_t>(), tmp.data(), tmp.size() * sizeof(int8_t));
        }
        if (dst_type == kINT4) {
            std::vector<int4_t> tmp = quant_vector<float, int4_t>(cpu_t.data<float>(), cpu_t.numel(), cpu_max);
            std::memcpy(ret._val.data<int4_t>(), tmp.data(), tmp.size() * sizeof(int4_t));
        }
    }
    return ret.to(t.dev());
};

inline int max_allclose(const QuantTensor& t0, const QuantTensor& t1, float rtol, float atol) {
    Tensor t0max = t0._max.to(kCPU);
    Tensor t1max = t1._max.to(kCPU);
    float v0 = quant_findmax<float>(t0max.data<float>(), t0max.numel(), nullptr);
    float v1 = quant_findmax<float>(t1max.data<float>(), t1max.numel(), nullptr);
    float abs_diff = fabs(v1 - v0);
    if (abs_diff <= atol + rtol * fabs(v1)) {
        return 1;
    } else {
        for (int i = 0; i < 8; i++) {
            std::cout << t0max.data<float>()[i] << "(" << t1max.data<float>()[i] << "), ";
        }
        std::cout << std::endl;
    }
    return 0;
};

inline int max_allclose(const Tensor& t0, const Tensor& t1, float rtol, float atol) {
    Tensor t0_cpu = t0.to(kCPU);
    Tensor t1_cpu = t1.to(kCPU);
    float v0 = quant_findmax<float>(t0_cpu.data<float>(), t0_cpu.numel(), nullptr);
    float v1 = quant_findmax<float>(t1_cpu.data<float>(), t1_cpu.numel(), nullptr);
    float abs_diff = fabs(v1 - v0);
    if (abs_diff <= atol + rtol * fabs(v1)) {
        return 1;
    } else {
        for (int i = 0; i < 8; i++) {
            std::cout << t0_cpu.data<float>()[i] << "(" << t1_cpu.data<float>()[i] << "), ";
        }
        std::cout << std::endl;
    }
    return 0;
};
}
}
}
#endif
