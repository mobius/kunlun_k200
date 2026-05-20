#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_VECTOR_UTIL_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_UTIL_VECTOR_UTIL_H
#include <vector>

inline int64_t vector_prod(const std::vector<int64_t>& vec, int begin, int end) {
    if (begin < 0 || end > (int)vec.size() || begin > end) {
        return -1;
    }

    int64_t prod = 1;
    for (int i = begin; i < end; i++) {
        prod = prod * vec[i];
    }
    return prod;
}

inline int64_t vector_prod(const std::vector<int64_t>& vec) {
    return vector_prod(vec, 0, vec.size());
}

inline int64_t vector_sum(const std::vector<int64_t>& vec) {
    int64_t sum = 0;
    for (size_t i = 0; i < vec.size(); i++) {
        sum = sum + vec[i];
    }
    return sum;
}

// TODO: need to remove initializer_list type when int64_t is ready for all ops
inline int64_t vector_prod(const std::initializer_list<int64_t>& vec, int begin, int end) {
    return vector_prod(std::vector<int64_t>(vec), begin, end);
}

// TODO: need to remove initializer_list type when int64_t is ready for all ops
inline int64_t vector_prod(const std::initializer_list<int64_t>& vec) {
    return vector_prod(std::vector<int64_t>(vec));
}

// TODO: need to remove initializer_list type when int64_t is ready for all ops
inline int64_t vector_sum(const std::initializer_list<int64_t>& vec) {
    return vector_sum(std::vector<int64_t>(vec));
}

// TODO: need to remove int type when int64_t is ready for all ops
inline int64_t vector_prod(const std::vector<int>& vec, int begin, int end) {
    if (begin < 0 || end > (int)vec.size() || begin > end) {
        return -1;
    }

    int64_t prod = 1;
    for (int i = begin; i < end; i++) {
        prod = prod * vec[i];
    }
    return prod;
}

// TODO: need to remove int type when int64_t is ready for all ops
inline int64_t vector_prod(const std::vector<int>& vec) {
    return vector_prod(vec, 0, vec.size());
}

// TODO: need to remove int type when int64_t is ready for all ops
inline int64_t vector_sum(const std::vector<int>& vec) {
    int64_t sum = 0;
    for (size_t i = 0; i < vec.size(); i++) {
        sum = sum + vec[i];
    }
    return sum;
}

inline float vector_sum(const std::vector<float>& vec) {
    float sum = 0;
    for (size_t i = 0; i < vec.size(); i++) {
        sum = sum + vec[i];
    }
    return sum;
}

inline std::vector<int64_t> id_to_split_id(const std::vector<int64_t>& dims, int64_t id) {
    std::vector<int64_t> ret;
    ret.resize(dims.size());
    for (int64_t i = dims.size() - 1; i >= 0; i--) {
        ret[i] = id % dims[i];
        id = id / dims[i];
    }
    return ret;
}

// TODO: need to remove int type when int64_t is ready for all ops
inline std::vector<int> id_to_split_id(const std::vector<int>& dims, int64_t id) {
    std::vector<int> ret;
    ret.resize(dims.size());
    for (int i = dims.size() - 1; i >= 0; i--) {
        ret[i] = id % dims[i];
        id = id / dims[i];
    }
    return ret;
}

// TODO: need to remove initializer_list type when int64_t is ready for all ops
inline std::vector<int64_t> id_to_split_id(const std::initializer_list<int64_t>& dims, int64_t id) {
    return id_to_split_id(std::vector<int64_t>(dims), id);
}
inline std::vector<int> id_to_split_id(const std::initializer_list<int>& dims, int64_t id) {
    return id_to_split_id(std::vector<int>(dims), id);
}

inline int64_t split_id_to_id(const std::vector<int64_t>& dims, const std::vector<int64_t>& split_id) {
    int64_t id = 0;
    for (size_t i = 0; i < dims.size(); i++) {
        id = id * dims[i];
        id = id + split_id[i];
    }
    return id;
}

// TODO: need to remove int type when int64_t is ready for all ops
inline int64_t split_id_to_id(const std::vector<int>& dims, const std::vector<int>& split_id) {
    int64_t id = 0;
    for (size_t i = 0; i < dims.size(); i++) {
        id = id * dims[i];
        id = id + split_id[i];
    }
    return id;
}

// TODO: need to remove initializer_list type when int64_t is ready for all ops
inline int64_t split_id_to_id(const std::initializer_list<int>& dims, const std::initializer_list<int>& split_id) {
    return split_id_to_id(std::vector<int>(dims), std::vector<int>(split_id));
}
inline int64_t split_id_to_id(const std::initializer_list<int64_t>& dims,
        const std::initializer_list<int64_t>& split_id) {
    return split_id_to_id(std::vector<int64_t>(dims), std::vector<int64_t>(split_id));
}

// target_len == 2 || target_len == 4 || target_len = 6
inline std::vector<int64_t> vector_extend(const std::vector<int64_t>& src, int target_len) {
    if (target_len == 2 && src.size() == 1) {
        return {src[0], src[0]};
    }
    if (target_len == 4 && src.size() == 1) {
        return {src[0], src[0], src[0], src[0]};
    }
    if (target_len == 4 && src.size() == 2) {
        return {src[0], src[0], src[1], src[1]};
    }
    // for conv3d
    if (target_len == 3 && src.size() == 1) {
        return {src[0], src[0], src[0]};
    }
    if (target_len == 6 && src.size() == 1) {
        return {src[0], src[0], src[0], src[0], src[0], src[0]};
    }
    if (target_len == 6 && src.size() == 2) {
        return {src[0], src[0], src[0], src[1], src[1], src[1]};
    }
    if (target_len == 6 && src.size() == 3) {
        return {src[0], src[0], src[1], src[1], src[2], src[2]};
    }
    return src;
}

// TODO: need to remove int type when int64_t is ready for all ops
// target_len == 2 || target_len == 4 || target_len = 6
inline std::vector<int> vector_extend(const std::vector<int>& src, int target_len) {
    if (target_len == 2 && src.size() == 1) {
        return {src[0], src[0]};
    }
    if (target_len == 4 && src.size() == 1) {
        return {src[0], src[0], src[0], src[0]};
    }
    if (target_len == 4 && src.size() == 2) {
        return {src[0], src[0], src[1], src[1]};
    }
    // for conv3d
    if (target_len == 3 && src.size() == 1) {
        return {src[0], src[0], src[0]};
    }
    if (target_len == 6 && src.size() == 1) {
        return {src[0], src[0], src[0], src[0], src[0], src[0]};
    }
    if (target_len == 6 && src.size() == 2) {
        return {src[0], src[0], src[0], src[1], src[1], src[1]};
    }
    if (target_len == 6 && src.size() == 3) {
        return {src[0], src[0], src[1], src[1], src[2], src[2]};
    }
    return src;
}

inline std::vector<int64_t> calc_conv2d_outsize(int64_t ih, int64_t iw, const std::vector<int64_t>& _ksize,
        const std::vector<int64_t>& _stride, const std::vector<int64_t>& _pad, const std::vector<int64_t>& _dilation) {
    std::vector<int64_t> ksize = vector_extend(_ksize, 2);
    std::vector<int64_t> stride = vector_extend(_stride, 2);
    std::vector<int64_t> pad = vector_extend(_pad, 4);
    std::vector<int64_t> dilation = vector_extend(_dilation, 2);
    int64_t oh = (ih + pad[0] + pad[1] - (dilation[0] * (ksize[0] - 1) + 1)) / stride[0] + 1;
    int64_t ow = (iw + pad[2] + pad[3] - (dilation[1] * (ksize[1] - 1) + 1)) / stride[1] + 1;
    return {oh, ow};
}

inline std::vector<int64_t> calc_conv3d_outsize(int64_t id, int64_t ih, int64_t iw, const std::vector<int64_t>& _ksize,
        const std::vector<int64_t>& _stride, const std::vector<int64_t>& _pad, const std::vector<int64_t>& _dilation) {
    std::vector<int64_t> ksize = vector_extend(_ksize, 3);
    std::vector<int64_t> stride = vector_extend(_stride, 3);
    std::vector<int64_t> pad = vector_extend(_pad, 6);
    std::vector<int64_t> dilation = vector_extend(_dilation, 3);
    int64_t od = (id + pad[0] + pad[1] - (dilation[0] * (ksize[0] - 1) + 1)) / stride[0] + 1;
    int64_t oh = (ih + pad[2] + pad[3] - (dilation[1] * (ksize[1] - 1) + 1)) / stride[1] + 1;
    int64_t ow = (iw + pad[4] + pad[5] - (dilation[2] * (ksize[2] - 1) + 1)) / stride[2] + 1;
    return {od, oh, ow};
}

inline std::vector<int64_t> calc_conv2d_transpose_outsize(int64_t ih, int64_t iw, const std::vector<int64_t>& _ksize,
        const std::vector<int64_t>& _stride, const std::vector<int64_t>& _pad, const std::vector<int64_t>& _dilation) {
    std::vector<int64_t> ksize = vector_extend(_ksize, 2);
    std::vector<int64_t> stride = vector_extend(_stride, 2);
    std::vector<int64_t> pad = vector_extend(_pad, 4);
    std::vector<int64_t> dilation = vector_extend(_dilation, 2);
    int64_t oh = (ih - 1) * stride[0] - pad[0] - pad[1] + (dilation[0] * (ksize[0] - 1) + 1);
    int64_t ow = (iw - 1) * stride[1] - pad[2] - pad[3] + (dilation[1] * (ksize[1] - 1) + 1);
    return {oh, ow};
}

inline std::vector<int64_t> calc_conv3d_transpose_outsize(int64_t id, int64_t ih, int64_t iw, const std::vector<int64_t>& _ksize,
        const std::vector<int64_t>& _stride, const std::vector<int64_t>& _pad, const std::vector<int64_t>& _dilation) {
    std::vector<int64_t> ksize = vector_extend(_ksize, 3);
    std::vector<int64_t> stride = vector_extend(_stride, 3);
    std::vector<int64_t> pad = vector_extend(_pad, 6);
    std::vector<int64_t> dilation = vector_extend(_dilation, 3);
    int64_t od = (id - 1) * stride[0] - pad[0] - pad[1] + (dilation[0] * (ksize[0] - 1) + 1);
    int64_t oh = (ih - 1) * stride[1] - pad[2] - pad[3] + (dilation[1] * (ksize[1] - 1) + 1);
    int64_t ow = (iw - 1) * stride[2] - pad[4] - pad[5] + (dilation[2] * (ksize[2] - 1) + 1);
    return {od, oh, ow};
}

inline std::vector<int64_t> calc_conv2d_outsize(int64_t ih, int64_t iw,
        const std::initializer_list<int64_t>& _ksize, const std::initializer_list<int64_t>& _stride,
        const std::initializer_list<int64_t>& _pad, const std::initializer_list<int64_t>& _dilation) {
    return calc_conv2d_outsize(ih, iw, std::vector<int64_t>(_ksize), std::vector<int64_t>(_stride),
            std::vector<int64_t>(_pad), std::vector<int64_t>(_dilation));
}

inline std::vector<int64_t> calc_conv3d_outsize(int64_t id, int64_t ih, int64_t iw, 
        const std::initializer_list<int64_t>& _ksize, const std::initializer_list<int64_t>& _stride,
        const std::initializer_list<int64_t>& _pad, const std::initializer_list<int64_t>& _dilation) {
    return calc_conv3d_outsize(id, ih, iw, std::vector<int64_t>(_ksize), std::vector<int64_t>(_stride),
            std::vector<int64_t>(_pad), std::vector<int64_t>(_dilation));
}


inline std::vector<int64_t> calc_conv2d_transpose_outsize(int64_t ih, int64_t iw,
        const std::initializer_list<int64_t>& _ksize, const std::initializer_list<int64_t>& _stride,
        const std::initializer_list<int64_t>& _pad, const std::initializer_list<int64_t>& _dilation) {
    return calc_conv2d_transpose_outsize(ih, iw, std::vector<int64_t>(_ksize), std::vector<int64_t>(_stride),
            std::vector<int64_t>(_pad), std::vector<int64_t>(_dilation));
}

inline std::vector<int64_t> calc_conv3d_transpose_outsize(int64_t id, int64_t ih, int64_t iw,
        const std::initializer_list<int64_t>& _ksize, const std::initializer_list<int64_t>& _stride,
        const std::initializer_list<int64_t>& _pad, const std::initializer_list<int64_t>& _dilation) {
    return calc_conv3d_transpose_outsize(id, ih, iw, std::vector<int64_t>(_ksize), std::vector<int64_t>(_stride),
            std::vector<int64_t>(_pad), std::vector<int64_t>(_dilation));
}

inline std::vector<int> calc_conv2d_outsize(int ih, int iw, const std::vector<int>& _ksize,
        const std::vector<int>& _stride, const std::vector<int>& _pad, const std::vector<int>& _dilation) {
    std::vector<int> ksize = vector_extend(_ksize, 2);
    std::vector<int> stride = vector_extend(_stride, 2);
    std::vector<int> pad = vector_extend(_pad, 4);
    std::vector<int> dilation = vector_extend(_dilation, 2);
    int oh = (ih + pad[0] + pad[1] - (dilation[0] * (ksize[0] - 1) + 1)) / stride[0] + 1;
    int ow = (iw + pad[2] + pad[3] - (dilation[1] * (ksize[1] - 1) + 1)) / stride[1] + 1;
    return {oh, ow};
}

inline std::vector<int> calc_conv3d_outsize(int id, int ih, int iw, const std::vector<int>& _ksize,
        const std::vector<int>& _stride, const std::vector<int>& _pad, const std::vector<int>& _dilation) {
    std::vector<int> ksize = vector_extend(_ksize, 3);
    std::vector<int> stride = vector_extend(_stride, 3);
    std::vector<int> pad = vector_extend(_pad, 6);
    std::vector<int> dilation = vector_extend(_dilation, 3);
    int od = (id + pad[0] + pad[1] - (dilation[0] * (ksize[0] - 1) + 1)) / stride[0] + 1;
    int oh = (ih + pad[2] + pad[3] - (dilation[1] * (ksize[1] - 1) + 1)) / stride[1] + 1;
    int ow = (iw + pad[4] + pad[5] - (dilation[2] * (ksize[2] - 1) + 1)) / stride[2] + 1;
    return {od, oh, ow};
}

inline std::vector<int> calc_conv2d_transpose_outsize(int ih, int iw, const std::vector<int>& _ksize,
        const std::vector<int>& _stride, const std::vector<int>& _pad, const std::vector<int>& _dilation) {
    std::vector<int> ksize = vector_extend(_ksize, 2);
    std::vector<int> stride = vector_extend(_stride, 2);
    std::vector<int> pad = vector_extend(_pad, 4);
    std::vector<int> dilation = vector_extend(_dilation, 2);
    int oh = (ih - 1) * stride[0] - pad[0] - pad[1] + (dilation[0] * (ksize[0] - 1) + 1);
    int ow = (iw - 1) * stride[1] - pad[2] - pad[3] + (dilation[1] * (ksize[1] - 1) + 1);
    return {oh, ow};
}

inline std::vector<int> calc_conv3d_transpose_outsize(int id, int ih, int iw, const std::vector<int>& _ksize,
        const std::vector<int>& _stride, const std::vector<int>& _pad, const std::vector<int>& _dilation) {
    std::vector<int> ksize = vector_extend(_ksize, 3);
    std::vector<int> stride = vector_extend(_stride, 3);
    std::vector<int> pad = vector_extend(_pad, 6);
    std::vector<int> dilation = vector_extend(_dilation, 3);
    int od = (id - 1) * stride[0] - pad[0] - pad[1] + (dilation[0] * (ksize[0] - 1) + 1);
    int oh = (ih - 1) * stride[1] - pad[2] - pad[3] + (dilation[1] * (ksize[1] - 1) + 1);
    int ow = (iw - 1) * stride[2] - pad[4] - pad[5] + (dilation[2] * (ksize[2] - 1) + 1);
    return {od, oh, ow};
}

inline std::vector<std::vector<int64_t>> vector2d_to_i64(const std::vector<std::vector<int>>& list) {
    std::vector<std::vector<int64_t>> list_i64;
    for (const auto& shape : list) {
        list_i64.emplace_back(std::vector<int64_t>(shape.begin(), shape.end()));
    }
    return list_i64;
}

#endif
