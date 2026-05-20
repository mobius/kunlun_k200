#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_MATH_VEC_INT_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_MATH_VEC_INT_H

#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
#include "xpu/refactor/deprecation/deprecated.h"
#include <memory>
#ifdef _MSC_VER
#ifdef max
#undef max
#endif
#ifdef min
#undef min
#endif
#endif
namespace baidu {
namespace xpu {
namespace api {
template<typename T> static inline int transpose(Context* ctx,
        const T* x, T* y, const std::vector<int>& xshape, const std::vector<int>& permute) {
    return transpose(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(permute.begin(), permute.end()));
}
template<typename T> static inline int reduce_sum(Context* ctx,
        const T* x, T* y, const std::vector<int>& xshape, const std::vector<int>& rdims) {
    return reduce_sum(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(rdims.begin(), rdims.end()));
}
template<typename T> static inline int reduce_mean(Context* ctx,
        const T* x, T* y, const std::vector<int>& xshape, const std::vector<int>& rdims) {
    return reduce_mean(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(rdims.begin(), rdims.end()));
}
template<typename T> static inline int reduce_max(Context* ctx,
        const T* x, T* y, const std::vector<int>& xshape, const std::vector<int>& rdims) {
    return reduce_max(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(rdims.begin(), rdims.end()));
}
template<typename T> static inline int reduce_min(Context* ctx,
        const T* x, T* y, const std::vector<int>& xshape, const std::vector<int>& rdims) {
    return reduce_min(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(rdims.begin(), rdims.end()));
}
template<typename T> static inline int reduce_prod(Context* ctx,
        const T* x, T* y, const std::vector<int>& xshape, const std::vector<int>& rdims) {
    return reduce_prod(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(rdims.begin(), rdims.end()));
}
template<typename T> static inline int reduce_L2(Context* ctx,
        const T* x, T* y, const std::vector<int>& xshape, const std::vector<int>& rdims) {
    return reduce_L2(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(rdims.begin(), rdims.end()));
}
template<typename T> static inline int reduce_all(Context* ctx,
        const T* x, T* y, const std::vector<int>& xshape, const std::vector<int>& rdims) {
    return reduce_all(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(rdims.begin(), rdims.end()));
}
template<typename T> static inline int reduce_any(Context* ctx,
        const T* x, T* y, const std::vector<int>& xshape, const std::vector<int>& rdims) {
    return reduce_any(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(rdims.begin(), rdims.end()));
}
template<typename T> static inline int broadcast(Context* ctx,
        const T* x, T* y, const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_add(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_add(ctx, x, y, z, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_sub(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_sub(ctx, x, y, z, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_mul(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_mul(ctx, x, y, z, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_div(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_div(ctx, x, y, z, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_max(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_max(ctx, x, y, z, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_min(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_min(ctx, x, y, z, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_pow(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_pow(ctx, x, y, z, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_mod(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_mod(ctx, x, y, z, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_floordiv(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_floordiv(ctx, x, y, z, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_equal(Context* ctx, const T* x, const T* y, bool* z,
        const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_equal(ctx, x, y, z, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_greater_than(Context* ctx, const T* x, const T* y, bool* z,
        const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_greater_than(ctx, x, y, z, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_greater_equal(Context* ctx, const T* x, const T* y, bool* z,
        const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_greater_equal(ctx, x, y, z, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_less_than(Context* ctx, const T* x, const T* y, bool* z,
        const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_less_than(ctx, x, y, z, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_less_equal(Context* ctx, const T* x, const T* y, bool* z,
        const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_less_equal(ctx, x, y, z, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_not_equal(Context* ctx, const T* x, const T* y, bool* z,
        const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_not_equal(ctx, x, y, z, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template <typename T> static inline int broadcast_addcdiv(Context* ctx, const T* w, const T* x, const T* y, T* z,
        const std::vector<int>& wshape, const std::vector<int>& xshape, const std::vector<int>& yshape, float scalar) {
    return broadcast_addcdiv(ctx, w, x, y, z, std::vector<int64_t>(wshape.begin(), wshape.end()),
            std::vector<int64_t>(xshape.begin(), xshape.end()), std::vector<int64_t>(yshape.begin(), yshape.end()),
            scalar);
}
template<typename T> static inline int broadcast_add_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_add_grad(ctx, x, y, z, dz, dy, dx, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_sub_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_sub_grad(ctx, x, y, z, dz, dy, dx, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_mul_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_mul_grad(ctx, x, y, z, dz, dy, dx, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_div_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_div_grad(ctx, x, y, z, dz, dy, dx, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_max_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_max_grad(ctx, x, y, z, dz, dy, dx, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int broadcast_min_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::vector<int>& xshape, const std::vector<int>& yshape) {
    return broadcast_min_grad(ctx, x, y, z, dz, dy, dx, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()));
}
template<typename T> static inline int meshgrid(Context* ctx, const std::vector<const T*>& x_list,
        const std::vector<T*>& y_list,
        const std::vector<std::vector<int>>& xshape_list) {
    return meshgrid(ctx, x_list, y_list, vvi32_to_vvi64(xshape_list));
}
template<typename T> static inline int index_select_grad(Context* ctx, const T* x, const int* index, const T* out_grad,
        int64_t dim, T* x_grad, const std::vector<int>& out_grad_shape, const std::vector<int>& x_grad_shape) {
    return index_select_grad(ctx, x, index, out_grad, dim, x_grad, std::vector<int64_t>(out_grad_shape.begin(),
            out_grad_shape.end()), std::vector<int64_t>(x_grad_shape.begin(), x_grad_shape.end()));
}
template<typename T> static inline int cumsum(Context* ctx, const T* x, T* y, const std::vector<int>& xshape,
        bool reverse, bool exclusive, int64_t axis) {
    return cumsum(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()), reverse, exclusive, axis);
}
template<typename T> static inline int logcumsumexp(Context* ctx, const T* x, T* y, const std::vector<int>& xshape,
        bool reverse, bool exclusive, int64_t axis) {
    return logcumsumexp(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()), reverse, exclusive, axis);
}
template<typename T> static inline int tile(Context* ctx,
        const T* x, T* y, const std::vector<int>& xshape, const std::vector<int>& expand) {
    return tile(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
                std::vector<int64_t>(expand.begin(), expand.end()));
}
template<typename T> static inline int concat(Context* ctx, const std::vector<const T*>& x_list, T* y,
        const std::vector<std::vector<int> >& xshape_list, int64_t axis) {
    return concat(ctx, x_list, y, vvi32_to_vvi64(xshape_list), axis);
}
template<typename T> static inline int split(Context* ctx, const T* x, const std::vector<T*>& y_list,
        const std::vector<int>& xshape, const std::vector<int>& split_list, int64_t axis) {
    return split(ctx, x, y_list, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(split_list.begin(), split_list.end()), axis);
}
template<typename T, typename TID> static inline int gather(Context* ctx, const T* x, const TID* index,
        T* y, const std::vector<int>& xshape, int64_t index_len, int64_t axis) {
    return gather(ctx, x, index, y, std::vector<int64_t>(xshape.begin(), xshape.end()), index_len, axis);
}
template<typename T, typename TID> static inline int gather_element(Context* ctx, const T* x, const TID* index, T* y,
        const std::vector<int>& xshape, const std::vector<int>& idxshape, int64_t axis) {
    return gather_element(ctx, x, index, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(idxshape.begin(), idxshape.end()), axis);
}
template<typename T, typename TID> static inline int gather_nd(Context* ctx, const T* x, const TID* index,
        T* y, const VectorParam<int>& xshape, const std::vector<int>& index_shape) {
    auto deleter = [](int64_t* ptr) {
        delete[] ptr;
    };
    std::shared_ptr<int64_t> xshape_i64(new int64_t[xshape.len], deleter);
    return gather_nd(ctx, x, index, y, vpi32_to_vpi64(xshape, xshape_i64.get()),
            std::vector<int64_t>(index_shape.begin(), index_shape.end()));
}
template<typename T, typename TID> static inline int gather_grad(Context* ctx, const T* dy, const TID* index,
        T* dx, const std::vector<int>& xshape, int64_t index_len, int64_t axis, bool overwrite = false) {
    return gather_grad(ctx, dy, index, dx, std::vector<int64_t>(xshape.begin(), xshape.end()),
            index_len, axis, overwrite);
}
template<typename T> static inline int pad(Context* ctx, const T* x, T* y, const std::vector<int>& xshape,
        const std::vector<int>& pad_left, const std::vector<int>& pad_right, T pad_value = 0) {
    return pad(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()), std::vector<int64_t>(pad_left.begin(),
            pad_left.end()), std::vector<int64_t>(pad_right.begin(), pad_right.end()), pad_value);
}
template<typename T> static inline int slice(Context* ctx,
        const T* x, T* y, const std::vector<int>& xshape, const std::vector<int>& starts, const std::vector<int>& ends) {
    return slice(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()), std::vector<int64_t>(starts.begin(), starts.end()),
            std::vector<int64_t>(ends.begin(), ends.end()));
}
template<typename T> static inline int as_strided(Context* ctx, const T* x, T* y, const std::vector<int>& yshape,
        const std::vector<int>& strides, const int& offset) {
    const int64_t offset_i64 = offset;
    return as_strided(ctx, x, y, std::vector<int64_t>(yshape.begin(), yshape.end()),
            std::vector<int64_t>(strides.begin(), strides.end()), offset_i64);
}
template<typename T> static inline int as_strided_view_update(Context* ctx, const T* x, T* y, const std::vector<int>& xshape,
        const std::vector<int>& strides, const int& offset) {
    const int64_t offset_i64 = offset;
    return as_strided_view_update(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(strides.begin(), strides.end()), offset_i64);
}
template<typename T> static inline int strided_slice(Context* ctx, const T* x, T* y, const std::vector<int>& xshape,
        const std::vector<int>& starts, const std::vector<int>& ends, const std::vector<int>& strides) {
    return strided_slice(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(starts.begin(), starts.end()), std::vector<int64_t>(ends.begin(), ends.end()),
            std::vector<int64_t>(strides.begin(), strides.end()));
}
template<typename T> static inline int strided_slice_view_update(Context* ctx, const T* x, T* y,
        const std::vector<int>& xshape, const std::vector<int>& yshape, const std::vector<int>& starts,
        const std::vector<int>& ends, const std::vector<int>& strides) {
    return strided_slice_view_update(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()), std::vector<int64_t>(yshape.begin(), yshape.end()),
            std::vector<int64_t>(starts.begin(), starts.end()), std::vector<int64_t>(ends.begin(), ends.end()),
            std::vector<int64_t>(strides.begin(), strides.end()));
}
template<typename T> static inline int strided_slice_grad(Context* ctx, const T* dy, T* dx, const std::vector<int>& xshape,
        const std::vector<int>& starts, const std::vector<int>& ends, const std::vector<int>& strides) {
    return strided_slice_grad(ctx, dy, dx, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(starts.begin(), starts.end()), std::vector<int64_t>(ends.begin(), ends.end()),
            std::vector<int64_t>(strides.begin(), strides.end()));
}
template<typename T> static inline int set_value(Context* ctx, const T* x, const T* value, T* y,
        const std::vector<int>& xshape, const std::vector<int>& value_shape, const std::vector<int>& starts, const std::vector<int>& ends,
        const std::vector<int>& steps, const std::vector<int>& axes, const std::vector<int>& decrease_axes = {}, const std::vector<int>& none_axes = {}) {
    return set_value(ctx, x, value, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(value_shape.begin(), value_shape.end()),
            std::vector<int64_t>(starts.begin(), starts.end()), std::vector<int64_t>(ends.begin(), ends.end()),
            std::vector<int64_t>(steps.begin(), steps.end()), std::vector<int64_t>(axes.begin(), axes.end()),
            std::vector<int64_t>(decrease_axes.begin(), decrease_axes.end()), std::vector<int64_t>(none_axes.begin(), none_axes.end()));
}
template<typename T> static inline int set_value_grad(Context* ctx, const T* dy, T* dx, T* dv,
        const std::vector<int>& xshape, const std::vector<int>& value_shape, const std::vector<int>& starts, const std::vector<int>& ends,
        const std::vector<int>& steps, const std::vector<int>& axes, const std::vector<int>& decrease_axes = {}, const std::vector<int>& none_axes = {}) {
    return set_value_grad(ctx, dy, dx, dv, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(value_shape.begin(), value_shape.end()),
            std::vector<int64_t>(starts.begin(), starts.end()), std::vector<int64_t>(ends.begin(), ends.end()),
            std::vector<int64_t>(steps.begin(), steps.end()), std::vector<int64_t>(axes.begin(), axes.end()),
            std::vector<int64_t>(decrease_axes.begin(), decrease_axes.end()), std::vector<int64_t>(none_axes.begin(), none_axes.end()));
}
template<typename T> static inline int where(Context* ctx, const T* x, int64_t* y, const std::vector<int>& xshape,
        int64_t nonzero_size) {
    return where(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()), nonzero_size);
}
template<typename T> static inline int select(Context* ctx, const bool* condition, const T* x, const T* y, T* z,
        const std::vector<int>& condition_shape, const std::vector<int>& xshape) {
    return select(ctx, condition, x, y, z, std::vector<int64_t>(condition_shape.begin(),
            condition_shape.end()), std::vector<int64_t>(xshape.begin(), xshape.end()));
}
template<typename T> static inline int select_grad(Context* ctx, const bool* condition, const T* grad,
        T* dx, T* dy, const std::vector<int>& condition_dims, const std::vector<int>& output_dims) {
    return select_grad(ctx, condition, grad, dx, dy, std::vector<int64_t>(condition_dims.begin(),
            condition_dims.end()), std::vector<int64_t>(output_dims.begin(), output_dims.end()));
}
template<typename T> static inline int masked_select(Context* ctx, const T* x, const bool* mask, T* y,
        const std::vector<int>& x_shape, const std::vector<int>& mask_shape, int64_t true_count) {
    return masked_select(ctx, x, mask, y, std::vector<int64_t>(x_shape.begin(), x_shape.end()),
            std::vector<int64_t>(mask_shape.begin(), mask_shape.end()), true_count);
}
template<typename T> static inline int masked_select_grad(Context* ctx, const T* y, const bool* mask, T* x,
        const std::vector<int>& x_shape, const std::vector<int>& mask_shape, int64_t true_count) {
    return masked_select_grad(ctx, y, mask, x, std::vector<int64_t>(x_shape.begin(), x_shape.end()),
            std::vector<int64_t>(mask_shape.begin(), mask_shape.end()), true_count);
}
template<typename T, typename TID> static inline int scatter(Context* ctx, const T* x, const T* updates, T* y,
        const VectorParam<TID>& index, const std::vector<int>& xshape, int64_t axis, bool is_overwrite) {
    return scatter(ctx, x, updates, y, index, std::vector<int64_t>(xshape.begin(), xshape.end()), axis,
            is_overwrite);
}
template<typename T, typename TID> static inline int scatter_element(Context* ctx, const T* x, const T* updates,
        const TID* index, T* y, const std::vector<int>& xyshape, const std::vector<int>& updshape,
        const std::vector<int>& idxshape, int64_t axis, int64_t reduction = 0) {
    return scatter_element(ctx, x, updates, index, y, std::vector<int64_t>(xyshape.begin(), xyshape.end()),
            std::vector<int64_t>(updshape.begin(), updshape.end()), std::vector<int64_t>(idxshape.begin(), idxshape.end()),
            axis, reduction);
}
template<typename T, typename TID> static inline int scatter_nd(Context* ctx, const T* x, const T* updates, T* y,
        const VectorParam<TID>& index, const VectorParam<int>& xshape, const std::vector<int>& index_shape,
        bool is_overwrite) {
    auto deleter = [](int64_t* ptr) {
        delete[] ptr;
    };
    std::shared_ptr<int64_t> xshape_i64(new int64_t[xshape.len], deleter);
    return scatter_nd(ctx, x, updates, y, index, vpi32_to_vpi64(xshape, xshape_i64.get()),
            std::vector<int64_t>(index_shape.begin(), index_shape.end()), is_overwrite);
}
template<typename T, typename TID> static inline int scatter_grad(Context* ctx, const T* dy,
        const VectorParam<TID>& index, T* dx, T* dupdates, const std::vector<int>& xshape, bool overwrite) {
    return scatter_grad(ctx, dy, index, dx, dupdates, std::vector<int64_t>(xshape.begin(), xshape.end()), overwrite);
}
template<typename T> static inline int flip(Context* ctx, const T* x, T* y,
        const std::vector<int>& xshape, const std::vector<int>& axis) {
    return flip(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(axis.begin(), axis.end()));
}
template<typename T> static inline int unbind(Context* ctx, const T* x, std::vector<T*>& y,
        const std::vector<int>& xshape, int64_t axis = 0) {
    return unbind(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()), axis);
}
template<typename TX, typename TY = int64_t> static inline int argmax(Context* ctx, const TX* x, TY* y,
        const std::vector<int>& xshape, int64_t axis) {
    return argmax(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()), axis);
}
template<typename TX, typename TY = int64_t> static inline int argmin(Context* ctx, const TX* x, TY* y,
        const std::vector<int>& xshape, int64_t axis) {
    return argmin(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()), axis);
}
template<typename T> static inline int fft2d(Context* ctx, const std::vector<int>& shape,
        const T* x_r, const T* x_i, T* y_r, T* y_i) {
    return fft2d(ctx, std::vector<int64_t>(shape.begin(), shape.end()), x_r, x_i, y_r, y_i);
}
template<typename T> static inline int fft3d(Context* ctx, const std::vector<int>& shape,
        const T* x_r, const T* x_i, T* y_r, T* y_i) {
    return fft3d(ctx, std::vector<int64_t>(shape.begin(), shape.end()), x_r, x_i, y_r, y_i);
}
template<typename T> static inline int logsumexp(Context* ctx, const T* x, T* y,
        const std::vector<int>& xshape, const std::vector<int>& axis) {
    return logsumexp(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(axis.begin(), axis.end()));
}
template<typename T> static inline int logsumexp_grad(Context* ctx, const T* x, const T* y, const T* dy,
        T* dx, const std::vector<int>& xshape, const std::vector<int>& axis_shape) {
    return logsumexp_grad(ctx, x, y, dy, dx, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(axis_shape.begin(), axis_shape.end()));
}
template<typename T> static inline int tril(Context* ctx, const T* x, T* y,
        const std::vector<int>& xshape, int64_t diagonal) {
    return tril(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()), diagonal);
}
template<typename T> static inline int triu(Context* ctx, const T* x, T* y,
        const std::vector<int>& xshape, int64_t diagonal) {
    return triu(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()), diagonal);
}
template<typename T> static inline int roll(Context* ctx, const T* x, T* y,
        const std::vector<int>& xshape, const std::vector<int>& shifts, const std::vector<int>& axis) {
    return roll(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()), std::vector<int64_t>(shifts.begin(),
            shifts.end()), std::vector<int64_t>(axis.begin(), axis.end()));
}
template<typename T> static inline int roll_grad(Context* ctx, const T* x, const T* out_grad, T* x_grad,
        const std::vector<int>& xshape, const std::vector<int>& shifts, const std::vector<int>& axis) {
    return roll_grad(ctx, x, out_grad, x_grad, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(shifts.begin(), shifts.end()), std::vector<int64_t>(axis.begin(),
            axis.end()));
}
template<typename T> static inline int diag(Context* ctx, const T* x, T* y,
        const std::vector<int>& xshape, const std::vector<int>& yshape, int64_t offset = 0, T padvalue = 0) {
    return diag(ctx, x, y, std::vector<int64_t>(xshape.begin(), xshape.end()),
            std::vector<int64_t>(yshape.begin(), yshape.end()), offset, padvalue);
}
template<typename T> static inline int transpose(Context* ctx,
        const T* x, T* y, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& permute) {
    return transpose(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(permute));
}
template<typename T> static inline int reduce_sum(Context* ctx,
        const T* x, T* y, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& rdims) {
    return reduce_sum(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(rdims));
}
template<typename T> static inline int reduce_mean(Context* ctx,
        const T* x, T* y, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& rdims) {
    return reduce_mean(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(rdims));
}
template<typename T> static inline int reduce_max(Context* ctx,
        const T* x, T* y, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& rdims) {
    return reduce_max(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(rdims));
}
template<typename T> static inline int reduce_min(Context* ctx,
        const T* x, T* y, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& rdims) {
    return reduce_min(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(rdims));
}
template<typename T> static inline int reduce_prod(Context* ctx,
        const T* x, T* y, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& rdims) {
    return reduce_prod(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(rdims));
}
template<typename T> static inline int reduce_L2(Context* ctx,
        const T* x, T* y, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& rdims) {
    return reduce_L2(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(rdims));
}
template<typename T> static inline int reduce_all(Context* ctx,
        const T* x, T* y, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& rdims) {
    return reduce_all(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(rdims));
}
template<typename T> static inline int reduce_any(Context* ctx,
        const T* x, T* y, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& rdims) {
    return reduce_any(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(rdims));
}
template<typename T> static inline int broadcast(Context* ctx,
        const T* x, T* y, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& yshape) {
    return broadcast(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_add(Context* ctx, const T* x, const T* y, T* z,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& yshape) {
    return broadcast_add(ctx, x, y, z, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_sub(Context* ctx, const T* x, const T* y, T* z,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& yshape) {
    return broadcast_sub(ctx, x, y, z, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_mul(Context* ctx, const T* x, const T* y, T* z,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& yshape) {
    return broadcast_mul(ctx, x, y, z, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_div(Context* ctx, const T* x, const T* y, T* z,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& yshape) {
    return broadcast_div(ctx, x, y, z, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_max(Context* ctx, const T* x, const T* y, T* z,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& yshape) {
    return broadcast_max(ctx, x, y, z, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_min(Context* ctx, const T* x, const T* y, T* z,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& yshape) {
    return broadcast_min(ctx, x, y, z, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_pow(Context* ctx, const T* x, const T* y, T* z,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& yshape) {
    return broadcast_pow(ctx, x, y, z, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_mod(Context* ctx, const T* x, const T* y, T* z,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& yshape) {
    return broadcast_mod(ctx, x, y, z, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_floordiv(Context* ctx, const T* x, const T* y, T* z,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& yshape) {
    return broadcast_floordiv(ctx, x, y, z, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_equal(Context* ctx, const T* x, const T* y, bool* z,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& yshape) {
    return broadcast_equal(ctx, x, y, z, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_greater_than(Context* ctx, const T* x, const T* y, bool* z,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& yshape) {
    return broadcast_greater_than(ctx, x, y, z, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_greater_equal(Context* ctx, const T* x, const T* y, bool* z,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& yshape) {
    return broadcast_greater_equal(ctx, x, y, z, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_less_than(Context* ctx, const T* x, const T* y, bool* z,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& yshape) {
    return broadcast_less_than(ctx, x, y, z, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_less_equal(Context* ctx, const T* x, const T* y, bool* z,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& yshape) {
    return broadcast_less_equal(ctx, x, y, z, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_not_equal(Context* ctx, const T* x, const T* y, bool* z,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& yshape) {
    return broadcast_not_equal(ctx, x, y, z, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape));
}

template <typename T> static inline int broadcast_addcdiv(Context* ctx, const T* w, const T* x, const T* y, T* z,
        const std::initializer_list<int64_t>& wshape, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& yshape, float scalar) {
    return broadcast_addcdiv(ctx, w, x, y, z, std::vector<int64_t>(wshape), std::vector<int64_t>(xshape),
            std::vector<int64_t>(yshape), scalar);
}
template<typename T> static inline int broadcast_add_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& yshape) {
    return broadcast_add_grad(ctx, x, y, z, dz, dy, dx, std::vector<int64_t>(xshape),
            std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_sub_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& yshape) {
    return broadcast_sub_grad(ctx, x, y, z, dz, dy, dx, std::vector<int64_t>(xshape),
            std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_mul_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& yshape) {
    return broadcast_mul_grad(ctx, x, y, z, dz, dy, dx, std::vector<int64_t>(xshape),
            std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_div_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& yshape) {
    return broadcast_div_grad(ctx, x, y, z, dz, dy, dx, std::vector<int64_t>(xshape),
            std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_max_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& yshape) {
    return broadcast_max_grad(ctx, x, y, z, dz, dy, dx, std::vector<int64_t>(xshape),
            std::vector<int64_t>(yshape));
}
template<typename T> static inline int broadcast_min_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& yshape) {
    return broadcast_min_grad(ctx, x, y, z, dz, dy, dx, std::vector<int64_t>(xshape),
            std::vector<int64_t>(yshape));
}
template<typename T> static inline int meshgrid(Context* ctx, const std::vector<const T*>& x_list,
        const std::vector<T*>& y_list,
        const std::initializer_list<std::initializer_list<int64_t>>& xshape_list) {
    return meshgrid(ctx, x_list, y_list, std::vector<std::vector<int64_t>>(xshape_list.begin(), xshape_list.end()));
}
template<typename T> int index_select_grad(Context* ctx, const T* x, const int* index, const T* out_grad,
        int64_t dim, T* x_grad, const std::initializer_list<int64_t>& out_grad_shape,
        const std::initializer_list<int64_t>& x_grad_shape) {
    return index_select_grad(ctx, x, index, out_grad, dim, x_grad, std::vector<int64_t>(out_grad_shape),
            std::vector<int64_t>(x_grad_shape));
}
template<typename T> int cumsum(Context* ctx, const T* x, T* y, const std::initializer_list<int64_t>& xshape,
        bool reverse, bool exclusive, int64_t axis) {
    return cumsum(ctx, x, y, std::vector<int64_t>(xshape), reverse, exclusive, axis);
}
template<typename T> static inline int logcumsumexp(Context* ctx, const T* x, T* y,
        const std::initializer_list<int64_t>& xshape, bool reverse, bool exclusive, int64_t axis) {
    return logcumsumexp(ctx, x, y, std::vector<int64_t>(xshape), reverse, exclusive, axis);
}
template<typename T> static inline int tile(Context* ctx,
        const T* x, T* y, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& expand) {
    return tile(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(expand));
}
template<typename T> static inline int concat(Context* ctx, const std::vector<const T*>& x_list, T* y,
        const std::initializer_list<std::initializer_list<int64_t> >& xshape_list, int64_t axis) {
    return concat(ctx, x_list, y, std::vector<std::vector<int64_t> >(xshape_list.begin(),
            xshape_list.end()), axis);
}
template<typename T> static inline int split(Context* ctx, const T* x, const std::vector<T*>& y_list,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& split_list,
        int64_t axis) {
    return split(ctx, x, y_list, std::vector<int64_t>(xshape), std::vector<int64_t>(split_list), axis);
}
template<typename T, typename TID> static inline int gather(Context* ctx, const T* x, const TID* index,
        T* y, const std::initializer_list<int64_t>& xshape, int64_t index_len, int64_t axis) {
    return gather(ctx, x, index, y, std::vector<int64_t>(xshape), index_len, axis);
}
template<typename T, typename TID> static inline int gather_element(Context* ctx, const T* x, const TID* index, T* y,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& idxshape, int64_t axis) {
    return gather_element(ctx, x, index, y, std::vector<int64_t>(xshape), std::vector<int64_t>(idxshape),
            axis);
}
template<typename T, typename TID> static inline int gather_nd(Context* ctx, const T* x, const TID* index,
        T* y, const VectorParam<int>& xshape, const std::initializer_list<int64_t>& index_shape) {
    auto deleter = [](int64_t* ptr) {
        delete[] ptr;
    };
    std::shared_ptr<int64_t> xshape_i64(new int64_t[xshape.len], deleter);
    return gather_nd(ctx, x, index, y, vpi32_to_vpi64(xshape, xshape_i64.get()), std::vector<int64_t>(index_shape));
}
template<typename T, typename TID> static inline int gather_grad(Context* ctx, const T* dy, const TID* index,
        T* dx, const std::initializer_list<int64_t>& xshape, int64_t index_len, int64_t axis, bool overwrite = false) {
    return gather_grad(ctx, dy, index, dx, std::vector<int64_t>(xshape), index_len, axis, overwrite);
}
template<typename T> static inline int pad(Context* ctx, const T* x, T* y, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& pad_left, const std::initializer_list<int64_t>& pad_right,
        T pad_value = 0) {
    return pad(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(pad_left),
            std::vector<int64_t>(pad_right), pad_value);
}
template<typename T> static inline int slice(Context* ctx,
        const T* x, T* y, const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& starts,
        const std::initializer_list<int64_t>& ends) {
    return slice(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(starts),
                std::vector<int64_t>(ends));
}
template<typename T> static inline int as_strided(Context* ctx, const T* x, T* y, const std::initializer_list<int64_t>& yshape,
        const std::initializer_list<int64_t>& strides, const int& offset) {
        const int64_t offset_i64 = offset;
    return as_strided(ctx, x, y, std::vector<int64_t>(yshape), std::vector<int64_t>(strides), offset_i64);
}
template<typename T> static inline int as_strided_view_update(Context* ctx, const T* x, T* y, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& strides, const int& offset) {
    const int64_t offset_i64 = offset;
    return as_strided_view_update(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(strides), offset_i64);
}
template<typename T> static inline int strided_slice(Context* ctx, const T* x, T* y, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& starts, const std::initializer_list<int64_t>& ends,
        const std::initializer_list<int64_t>& strides) {
    return strided_slice(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(starts),
            std::vector<int64_t>(ends), std::vector<int64_t>(strides));
}
template<typename T> static inline int strided_slice_view_update(Context* ctx, const T* x, T* y,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& yshape, const std::initializer_list<int64_t>& starts,
        const std::initializer_list<int64_t>& ends, const std::initializer_list<int64_t>& strides) {
    return strided_slice_view_update(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape),
            std::vector<int64_t>(starts), std::vector<int64_t>(ends), std::vector<int64_t>(strides));
}
template<typename T> static inline int strided_slice_grad(Context* ctx, const T* dy, T* dx, const std::initializer_list<int64_t>& xshape,
        const std::initializer_list<int64_t>& starts, const std::initializer_list<int64_t>& ends, const std::initializer_list<int64_t>& strides) {
    return strided_slice_grad(ctx, dy, dx, std::vector<int64_t>(xshape), std::vector<int64_t>(starts),
            std::vector<int64_t>(ends), std::vector<int64_t>(strides));
}
template<typename T> static inline int set_value(Context* ctx, const T* x, const T* value, T* y,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& value_shape,
        const std::initializer_list<int64_t>& starts, const std::initializer_list<int64_t>& ends,
        const std::initializer_list<int64_t>& steps, const std::initializer_list<int64_t>& axes,
        const std::initializer_list<int64_t>& decrease_axes = {}, const std::initializer_list<int64_t>& none_axes = {}) {
    return set_value(ctx, x, value, y, std::vector<int64_t>(xshape), std::vector<int64_t>(value_shape),
            std::vector<int64_t>(starts), std::vector<int64_t>(ends), std::vector<int64_t>(steps),
            std::vector<int64_t>(axes), std::vector<int64_t>(decrease_axes), std::vector<int64_t>(none_axes));
}
template<typename T> static inline int set_value_grad(Context* ctx, const T* dy, T* dx, T* dv,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& value_shape,
        const std::initializer_list<int64_t>& starts, const std::initializer_list<int64_t>& ends,
        const std::initializer_list<int64_t>& steps, const std::initializer_list<int64_t>& axes,
        const std::initializer_list<int64_t>& decrease_axes = {}, const std::initializer_list<int64_t>& none_axes = {}) {
    return set_value_grad(ctx, dy, dx, dv, std::vector<int64_t>(xshape), std::vector<int64_t>(value_shape),
            std::vector<int64_t>(starts), std::vector<int64_t>(ends), std::vector<int64_t>(steps),
            std::vector<int64_t>(axes), std::vector<int64_t>(decrease_axes), std::vector<int64_t>(none_axes));
}
template<typename T> static inline int where(Context* ctx, const T* x, int64_t* y, const std::initializer_list<int64_t>& xshape,
        int64_t nonzero_size) {
    return where(ctx, x, y, std::vector<int64_t>(xshape), nonzero_size);
}
template<typename T> static inline int select(Context* ctx, const bool* condition, const T* x, const T* y, T* z,
        const std::initializer_list<int64_t>& condition_shape, const std::initializer_list<int64_t>& xshape) {
    return select(ctx, condition, x, y, z, std::vector<int64_t>(condition_shape),
            std::vector<int64_t>(xshape));
}
template<typename T> static inline int select_grad(Context* ctx, const bool* condition, const T* grad,
        T* dx, T* dy, const std::initializer_list<int64_t>& condition_dims,
        const std::initializer_list<int64_t>& output_dims) {
    return select_grad(ctx, condition, grad, dx, dy, std::vector<int64_t>(condition_dims),
            std::vector<int64_t>(output_dims));
}
template<typename T> static inline int masked_select(Context* ctx, const T* x, const bool* mask, T* y,
        const std::initializer_list<int64_t>& x_shape, const std::initializer_list<int64_t>& mask_shape,
        int64_t true_count) {
    return masked_select(ctx, x, mask, y, std::vector<int64_t>(x_shape), std::vector<int64_t>(mask_shape),
            true_count);
}
template<typename T> static inline int masked_select_grad(Context* ctx, const T* y, const bool* mask, T* x,
        const std::initializer_list<int64_t>& x_shape, const std::initializer_list<int64_t>& mask_shape,
        int64_t true_count) {
    return masked_select_grad(ctx, y, mask, x, std::vector<int64_t>(x_shape), std::vector<int64_t>(mask_shape),
            true_count);
}
template<typename T, typename TID> static inline int scatter(Context* ctx, const T* x, const T* updates, T* y,
        const VectorParam<TID>& index, const std::initializer_list<int64_t>& xshape, int64_t axis, bool is_overwrite) {
    return scatter(ctx, x, updates, y, index, std::vector<int64_t>(xshape), axis, is_overwrite);
}
template<typename T, typename TID> static inline int scatter_element(Context* ctx, const T* x, const T* updates,
        const TID* index, T* y, const std::initializer_list<int64_t>& xyshape, const std::initializer_list<int64_t>& updshape,
        const std::initializer_list<int64_t>& idxshape, int64_t axis) {
    return scatter_element(ctx, x, updates, index, y, std::vector<int64_t>(xyshape), std::vector<int64_t>(updshape),
            std::vector<int64_t>(idxshape), axis);
}
template<typename T, typename TID> static inline int scatter_nd(Context* ctx, const T* x, const T* updates, T* y,
        const VectorParam<TID>& index, const VectorParam<int>& xshape, const std::initializer_list<int64_t>& index_shape,
        bool is_overwrite) {
    auto deleter = [](int64_t* ptr) {
        delete[] ptr;
    };
    std::shared_ptr<int64_t> xshape_i64(new int64_t[xshape.len], deleter);
    return scatter_nd(ctx, x, updates, y, index, vpi32_to_vpi64(xshape, xshape_i64.get()),
            std::vector<int64_t>(index_shape), is_overwrite);
}
template<typename T, typename TID> static inline int scatter_grad(Context* ctx, const T* dy,
        const VectorParam<TID>& index, T* dx, T* dupdates, const std::initializer_list<int64_t>& xshape,
        bool overwrite) {
    return scatter_grad(ctx, dy, index, dx, dupdates, std::vector<int64_t>(xshape), overwrite);
}
template<typename T> static inline int flip(Context* ctx, const T* x, T* y,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& axis) {
    return flip(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(axis));
}
template<typename T> static inline int unbind(Context* ctx, const T* x, std::initializer_list<T*>& y,
        const std::initializer_list<int64_t>& xshape, int64_t axis = 0) {
    return unbind(ctx, x, y, std::vector<int64_t>(xshape), axis);
}
template<typename TX, typename TY = int64_t> static inline int argmax(Context* ctx, const TX* x, TY* y,
        const std::initializer_list<int64_t>& xshape, int64_t axis) {
    return argmax(ctx, x, y, std::vector<int64_t>(xshape), axis);
}
template<typename TX, typename TY = int64_t> static inline int argmin(Context* ctx, const TX* x, TY* y,
        const std::initializer_list<int64_t>& xshape, int64_t axis) {
    return argmin(ctx, x, y, std::vector<int64_t>(xshape), axis);
}
template<typename T> static inline int fft2d(Context* ctx, const std::initializer_list<int64_t>& shape,
        const T* x_r, const T* x_i, T* y_r, T* y_i) {
    return fft2d(ctx, std::vector<int64_t>(shape), x_r, x_i, y_r, y_i);
}
template<typename T> static inline int fft3d(Context* ctx, const std::initializer_list<int64_t>& shape,
        const T* x_r, const T* x_i, T* y_r, T* y_i) {
    return fft3d(ctx, std::vector<int64_t>(shape), x_r, x_i, y_r, y_i);
}
template<typename T> static inline int logsumexp(Context* ctx, const T* x, T* y,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& axis) {
   return logsumexp(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(axis));
}
template<typename T> static inline int logsumexp_grad(Context* ctx, const T* x, const T* y, const T* dy,
        T* dx, const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& axis_shape) {
    return logsumexp_grad(ctx, x, y, dy, dx, std::vector<int64_t>(xshape), std::vector<int64_t>(axis_shape));
}
template<typename T> static inline int tril(Context* ctx, const T* x, T* y,
        const std::initializer_list<int64_t>& xshape, int64_t diagonal) {
    return tril(ctx, x, y, std::vector<int64_t>(xshape), diagonal);
}
template<typename T> static inline int triu(Context* ctx, const T* x, T* y,
        const std::initializer_list<int64_t>& xshape, int64_t diagonal) {
    return triu(ctx, x, y, std::vector<int64_t>(xshape), diagonal);
}
template<typename T> static inline int roll(Context* ctx, const T* x, T* y,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& shifts,
        const std::initializer_list<int64_t>& axis) {
    return roll(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(shifts), std::vector<int64_t>(axis));
}
template<typename T> static inline int roll_grad(Context* ctx, const T* x, const T* out_grad, T* x_grad,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& shifts,
        const std::initializer_list<int64_t>& axis) {
    return roll_grad(ctx, x, out_grad, x_grad, std::vector<int64_t>(xshape),
            std::vector<int64_t>(shifts), std::vector<int64_t>(axis));
}
template<typename T> static inline int diag(Context* ctx, const T* x, T* y,
        const std::initializer_list<int64_t>& xshape, const std::initializer_list<int64_t>& yshape,
        int64_t offset = 0, T padvalue = 0) {
    return diag(ctx, x, y, std::vector<int64_t>(xshape), std::vector<int64_t>(yshape), offset, padvalue);
}
template<typename T> static inline int sorted_topk_with_filter(Context* ctx, const T* x, T* y,
        int* index, int64_t m, int64_t n, int64_t k, bool largest, const float x_threshold, int& k_filtered) {
    int64_t k_filtered_i64 = static_cast<int64_t>(k_filtered);
    int ret = sorted_topk_with_filter<T, int>(ctx, x, y, index, m, n, k, largest, x_threshold, k_filtered_i64);
    k_filtered = static_cast<int>(k_filtered_i64);
    return ret;
}

}
}
}
#endif
