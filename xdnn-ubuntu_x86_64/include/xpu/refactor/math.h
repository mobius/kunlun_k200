#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_MATH_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_MATH_H

#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
#include "xpu/refactor/deprecation/deprecated.h"
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

// quant
template<typename T> DLL_EXPORT int findmax(Context* ctx, const T* x, float* maxptr, int64_t len);
template <typename T> DLL_EXPORT int findmax2d(Context* ctx, const T* x, float* maxptr, int64_t m, int64_t n, int64_t ldx);
template<typename T> DLL_EXPORT int quant_loss(Context* ctx, const float* x, float* y, int64_t len);
template<typename TX, typename TY> DLL_EXPORT int quantization_pb(Context* ctx,
        const TX* x, TY* y, int64_t m, int64_t n, const float* maxptr);
template<typename TX, typename TY> DLL_EXPORT int quantization(Context* ctx,
        const TX* x, TY* y, int64_t len, const float* maxptr);
template<typename TX> DLL_EXPORT int quant2d(Context* ctx,
        const TX* x, int8_t* y, float* scale, int m, int n);
template<typename TX, typename TY> DLL_EXPORT int dequantization(Context* ctx,
        const TX* x, TY* y, int64_t len, const float* maxptr);
// vsl version
template<typename T> DLL_EXPORT int batch_findmax(Context* ctx, const T* x, int row, int col,
        int batch_size, const int* lod, float* batch_maxvalue);
// novsl version
template<typename T> DLL_EXPORT int batch_findmax(Context* ctx, const T* x, int64_t batch,
        int64_t len_per_batch, float* batch_maxvalue);
// for fc weight only
template<typename TX, typename TY>
DLL_EXPORT int gpt_quant_weight_only(Context* ctx, const TX* x, TY* y,
        int64_t len, const float* maxptr);
DLL_EXPORT int gpt_fp16_quant_2int8(Context* ctx, const float16* x, int8_t* y,
        int64_t len, const float* maxptr);
// for int4_t weight
template<typename TX>
DLL_EXPORT int gpt_weight_reori(Context* ctx, const TX* x, TX* y,
        int64_t k, int64_t n, int64_t ldw, bool w_trans);
// basic op
template<typename T> DLL_EXPORT int constant(Context* ctx, T* x, int64_t len, T val);
template<typename T> DLL_EXPORT int copy(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int copy2d(Context* ctx, const T* x, T* y, int64_t m, int64_t ldx, int64_t ldy, int64_t n);
template<typename TX, typename TY> DLL_EXPORT int cast(Context* ctx, const TX* x, TY* y, int64_t len);
DLL_EXPORT int cast_te(Context* ctx, const bfloat16* x, const float* abs_max_x, float16* y, float* scale, int64_t len);
template<typename T> DLL_EXPORT int range(Context* ctx, T* y, T begin, T step, int64_t len);
template<typename T> DLL_EXPORT int any(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int stack(Context* ctx, const std::vector<const T*>& x_ptrs, T* y, int64_t height,
        int64_t width);
template<typename T> DLL_EXPORT int fill_diagonal(Context* ctx, const T* x, T* y, const std::vector<int64_t>& shape, T value = 0, int64_t offset = 0, bool wrap = false);
template<typename T> DLL_EXPORT int diagonal(Context* ctx, const T* x, T* y,
            const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape, int64_t axis0 = 0, int64_t axis1 = 1, int64_t offset = 0);
template<typename T> DLL_EXPORT int expand_grad(Context* ctx, const T* dy, T* dx, const std::vector<int64_t>& dyshape,
                   const std::vector<int64_t>& dxshape);
template<typename T> DLL_EXPORT int fill_diagonal_tensor(Context* ctx, const T* x, const T* value, T* y, const std::vector<int64_t>& xshape, const std::vector<int64_t>& vshape,
                int64_t axis0 = 0, int64_t axis1 = 1, int64_t offset = 0);
template<typename T> DLL_EXPORT int linspace(Context* ctx, T* y, T start, T stop, int64_t num);
template<typename T> DLL_EXPORT int lerp(Context* ctx, const T* x, const T* y,const T* w, T* z, const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape,
        const std::vector<int64_t>& wshape);
template<typename T> DLL_EXPORT int trace(Context* ctx, const T* x, T* y, const std::vector<int64_t>& xshape);
template<typename T, typename TID> DLL_EXPORT int index_add(Context* ctx, const T* x, const T* src, T* y, const TID* index,
        const std::vector<int64_t>& xshape, int64_t idx_len, int64_t axis, T alpha);
template<typename T> DLL_EXPORT int eye(Context* ctx, T* x, int64_t n, int64_t m);
template<typename T> DLL_EXPORT int exp2(Context* ctx, const T* x, T* y, int64_t len);

// unary op
template<typename T> DLL_EXPORT int scale(Context* ctx, const T* x, T* y, int64_t len,
        bool bias_after_scale, float _scale, float _bias);
template<typename T, typename TID = int> DLL_EXPORT int index_select_grad(Context* ctx, const T* x, const TID* index, const T* out_grad,
        int64_t dim, T* x_grad, const std::vector<int64_t>& out_grad_shape, const std::vector<int64_t>& x_grad_shape);
template<typename T> DLL_EXPORT int exp_grad(Context* ctx, const T* x, const T* out, const T* out_grad, T* x_grad, int64_t len);
template<typename T> DLL_EXPORT int abs(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int log(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int log1p(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int sqrt(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int exp(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int neg(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int square(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int rsqrt(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int sign(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int erf(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int erfinv(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int reciprocal(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int clip_grad(Context* ctx, const T* x, const T* dy, T* dx, int64_t len, T min_val, T max_val);
template<typename T> DLL_EXPORT int clip(Context* ctx, const T* x, T* y, int64_t len, T min_val, T max_val);
template<typename T> DLL_EXPORT int sqrt_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int rsqrt_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int square_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int reciprocal_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int abs_grad(Context* ctx, const T* x, const T* y, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int abs_grad_grad(Context* ctx, const T* x, const T* ddx, T* ddy, int64_t len);
template<typename T> DLL_EXPORT int cumsum(Context* ctx, const T* x, T* y, const std::vector<int64_t>& xshape,
        bool reverse, bool exclusive, int64_t axis);
template<typename T> DLL_EXPORT int cumprod(Context* ctx, const T* x, T* y, const std::vector<int64_t>& xshape, int64_t axis);
template<typename T> DLL_EXPORT int logcumsumexp(Context* ctx, const T* x, T* y, const std::vector<int64_t>& xshape,
        bool reverse, bool exclusive, int64_t axis);
template<typename T> DLL_EXPORT int repeat_interleave(Context* ctx, const T* x, T* y, int64_t x_len, int64_t y_len);
// is infinity or is nans
template<typename T> DLL_EXPORT int isfinite(Context* ctx, const T* x, int8_t* y, int64_t len);
template<typename T> DLL_EXPORT int isnan(Context* ctx, const T* x, int8_t* y, int64_t len);
template<typename T> DLL_EXPORT int isfinite(Context* ctx, const T* x, bool* y, int64_t len);
template<typename T> DLL_EXPORT int isnan(Context* ctx, const T* x, bool* y, int64_t len);
// count infinity or nans
template<typename T, typename TID = int> DLL_EXPORT int count_nan_or_inf(Context* ctx, const T* x, TID* y, int64_t len);
template<typename T> DLL_EXPORT int check_nan_or_inf(Context* ctx, const T* x, bool* y, int64_t len);
// binary op
template<typename T> DLL_EXPORT int add(Context* ctx, const T* x, const T* y, T* z, int64_t len);
template<typename T> DLL_EXPORT int sub(Context* ctx, const T* x, const T* y, T* z, int64_t len);
template<typename T> DLL_EXPORT int mul(Context* ctx, const T* x, const T* y, T* z, int64_t len);
template<typename T> DLL_EXPORT int div(Context* ctx, const T* x, const T* y, T* z, int64_t len);
template<typename T> DLL_EXPORT int max(Context* ctx, const T* x, const T* y, T* z, int64_t len);
template<typename T> DLL_EXPORT int min(Context* ctx, const T* x, const T* y, T* z, int64_t len);
template<typename T> DLL_EXPORT int pow(Context* ctx, const T* x, const T* y, T* z, int64_t len);
template<typename T> DLL_EXPORT int floordiv(Context* ctx, const T* x, const T* y, T* z, int64_t len);
template <typename T> DLL_EXPORT int addcmul(Context* ctx, const T* w, const T* x, const T* y, T* z, float scalar, int64_t len);
template <typename T> DLL_EXPORT int addcdiv(Context* ctx, const T* w, const T* x, const T* y, T* z, float scalar, int64_t len);
template<typename T> DLL_EXPORT int add_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dx, T* dy, int64_t len);
template<typename T> DLL_EXPORT int sub_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dx, T* dy, int64_t len);
template<typename T> DLL_EXPORT int mul_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dx, T* dy, int64_t len);
template<typename T> DLL_EXPORT int div_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dx, T* dy, int64_t len);
template<typename T> DLL_EXPORT int max_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dx, T* dy, int64_t len);
template<typename T> DLL_EXPORT int min_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dx, T* dy, int64_t len);
template<typename T> DLL_EXPORT int pow_grad(Context* ctx, const T* x, const T* dy, T* dx, int64_t len, float factor);
template<typename T> DLL_EXPORT int ceil(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int floor(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int round(Context* ctx, const T* x, T* y, int64_t len, int64_t decimals);
template<typename T> DLL_EXPORT int fmod(Context* ctx, const T* x, const T* y, T* z, int64_t len);
template<typename T> DLL_EXPORT int remainder(Context* ctx, const T* x, const T* y, T* z, int64_t len);
// trigonometric op
template<typename T> DLL_EXPORT int sin(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int cos(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int tan(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int arcsin(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int arccos(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int arctan(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int arctan2(Context* ctx, const T* x, const T* x2, T* y, int64_t len);
template<typename T> DLL_EXPORT int sin_grad(Context* ctx, const T* x, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int cos_grad(Context* ctx, const T* x, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int tan_grad(Context* ctx, const T* x, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int arcsin_grad(Context* ctx, const T* x, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int arccos_grad(Context* ctx, const T* x, const T* dy, T* dx, int64_t len);
template<typename T> DLL_EXPORT int arctan_grad(Context* ctx, const T* x, const T* dy, T* dx, int64_t len);
// hyperbolic op
template<typename T> DLL_EXPORT int sinh(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int cosh(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int coth(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int sech(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int csch(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int asinh(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int acosh(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int atanh(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int acoth(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int asech(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int acsch(Context* ctx, const T* x, T* y, int64_t len);

//compare op
template<typename T> DLL_EXPORT int equal(Context* ctx, const T* x, const T* y, bool* z, int64_t len);
template<typename T> DLL_EXPORT int not_equal(Context* ctx, const T* x, const T* y, bool* z, int64_t len);
template<typename T> DLL_EXPORT int less_than(Context* ctx, const T* x, const T* y, bool* z, int64_t len);
template<typename T> DLL_EXPORT int less_equal(Context* ctx, const T* x, const T* y, bool* z, int64_t len);
template<typename T> DLL_EXPORT int greater_than(Context* ctx, const T* x, const T* y, bool* z, int64_t len);
template<typename T> DLL_EXPORT int greater_equal(Context* ctx, const T* x, const T* y, bool* z, int64_t len);
//logical op
template<typename T> DLL_EXPORT int logical_not(Context* ctx, const T* x, T* y, int64_t len);
template<typename T> DLL_EXPORT int logical_and(Context* ctx, const T* x, const T* y, T* z, int64_t len);
template<typename T> DLL_EXPORT int logical_or(Context* ctx, const T* x, const T* y, T* z, int64_t len);
template<typename T> DLL_EXPORT int logical_xor(Context* ctx, const T* x, const T* y, T* z, int64_t len);
// reduce op
template<typename T> DLL_EXPORT int reduce_sum(Context* ctx,
        const T* x, T* y, const std::vector<int64_t>& xshape, const std::vector<int64_t>& rdims);
template<typename T> DLL_EXPORT int reduce_mean(Context* ctx,
        const T* x, T* y, const std::vector<int64_t>& xshape, const std::vector<int64_t>& rdims);
template<typename T> DLL_EXPORT int reduce_max(Context* ctx,
        const T* x, T* y, const std::vector<int64_t>& xshape, const std::vector<int64_t>& rdims);
template<typename T> DLL_EXPORT int reduce_min(Context* ctx,
        const T* x, T* y, const std::vector<int64_t>& xshape, const std::vector<int64_t>& rdims);
template<typename T> DLL_EXPORT int reduce_prod(Context* ctx,
        const T* x, T* y, const std::vector<int64_t>& xshape, const std::vector<int64_t>& rdims);
template<typename T> DLL_EXPORT int reduce_L2(Context* ctx,
        const T* x, T* y, const std::vector<int64_t>& xshape, const std::vector<int64_t>& rdims);
template<typename T> DLL_EXPORT int reduce_all(Context* ctx,
        const T* x, T* y, const std::vector<int64_t>& xshape, const std::vector<int64_t>& rdims);
template<typename T> DLL_EXPORT int reduce_any(Context* ctx,
        const T* x, T* y, const std::vector<int64_t>& xshape, const std::vector<int64_t>& rdims);
template<typename T> DLL_EXPORT int mean_all_grad(Context* ctx, const T* dy, T* dx, int64_t len);

// data movement
template<typename T> DLL_EXPORT int tile(Context* ctx,
        const T* x, T* y, const std::vector<int64_t>& xshape, const std::vector<int64_t>& expand);
template<typename T> DLL_EXPORT int broadcast(Context* ctx,
        const T* x, T* y, const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int meshgrid(Context* ctx, const std::vector<const T*>& x_list,
        const std::vector<T*>& y_list,
        const std::vector<std::vector<int64_t> >& xshape_list);
template<typename T> DLL_EXPORT int transpose(Context* ctx,
        const T* x, T* y, const std::vector<int64_t>& xshape, const std::vector<int64_t>& permute);
template<typename T> DLL_EXPORT int concat(Context* ctx, const std::vector<const T*>& x_list, T* y,
        const std::vector<std::vector<int64_t> >& xshape_list, int64_t axis);
template<typename T> DLL_EXPORT int split(Context* ctx, const T* x, const std::vector<T*>& y_list,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& split_list, int64_t axis);
template<typename T, typename TID> DLL_EXPORT int gather(Context* ctx, const T* x, const TID* index,
        T* y, const std::vector<int64_t>& xshape, int64_t index_len, int64_t axis);
template <typename T, typename TID>
DLL_EXPORT int gather_part(Context* ctx,
                           const T* x,
                           const TID* index,
                           T* y,
                           const std::vector<int64_t>& xshape,
                           int64_t index_len,
                           int64_t axis,
                           int64_t axis_1,
                           int64_t offset,
                           int64_t gather_len);
template<typename T, typename TID> DLL_EXPORT int gather_element(Context* ctx, const T* x, const TID* index, T* y,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& idxshape, int64_t axis);
template<typename T, typename TID> DLL_EXPORT int gather_nd(Context* ctx, const T* x, const TID* index,
        T* y, const VectorParam<int64_t>& xshape, const std::vector<int64_t>& index_shape);
template<typename T, typename TID> DLL_EXPORT int gather_nd_int(Context* ctx, const T* x, const TID* index,
        T* y, const VectorParam<int64_t>& xshape, const std::vector<int64_t>& index_shape);
template<typename T, typename TID> DLL_EXPORT int gather_grad(Context* ctx, const T* dy, const TID* index,
        T* dx, const std::vector<int64_t>& xshape, int64_t index_len, int64_t axis, bool overwrite = false);
template<typename T> DLL_EXPORT int pad(Context* ctx, const T* x, T* y, const std::vector<int64_t>& xshape,
        const std::vector<int64_t>& pad_left, const std::vector<int64_t>& pad_right, T pad_value = 0);
template<typename T> DLL_EXPORT int slice(Context* ctx,
        const T* x, T* y, const std::vector<int64_t>& xshape, const std::vector<int64_t>& starts,
        const std::vector<int64_t>& ends);
template<typename T> DLL_EXPORT int strided_slice(Context* ctx, const T* x, T* y, const std::vector<int64_t>& xshape,
        const std::vector<int64_t>& starts, const std::vector<int64_t>& ends, const std::vector<int64_t>& strides, int64_t offset = 0);
template<typename T> DLL_EXPORT int strided_slice_view_update(Context* ctx, const T* x, T* y,
        const std::vector<int64_t>& xshape,
        const std::vector<int64_t>& yshape,
        const std::vector<int64_t>& starts,
        const std::vector<int64_t>& ends,
        const std::vector<int64_t>& strides);
template<typename T> DLL_EXPORT int strided_slice_grad(Context* ctx, const T* dy, T* dx, const std::vector<int64_t>& xshape,
        const std::vector<int64_t>& starts, const std::vector<int64_t>& ends, const std::vector<int64_t>& strides);
template <typename T> DLL_EXPORT int as_strided(Context* ctx, const T* x, T* y,
        const std::vector<int64_t>& yshape,
        const std::vector<int64_t>& strides,
        const int64_t& offset);
template <typename T> DLL_EXPORT int as_strided_view_update(Context* ctx, const T* x, T* y,
        const std::vector<int64_t>& xshape,
        const std::vector<int64_t>& strides,
        const int64_t& offset);
template <typename T> DLL_EXPORT int strided_copy(Context* ctx, const T* x, T* y,
        const std::vector<int64_t>& xshape,
        const std::vector<int64_t>& yshape,
        const std::vector<int64_t>& xstrides,
        const std::vector<int64_t>& ystrides);
template<typename T> DLL_EXPORT int set_value(Context* ctx, const T* x, const T* value, T* y,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& value_shape, const std::vector<int64_t>& starts, const std::vector<int64_t>& ends,
        const std::vector<int64_t>& steps, const std::vector<int64_t>& axes, const std::vector<int64_t>& decrease_axes = {}, const std::vector<int64_t>& none_axes = {});
template<typename T> DLL_EXPORT int set_value_grad(Context* ctx, const T* dy, T* dx, T* dv,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& value_shape, const std::vector<int64_t>& starts, const std::vector<int64_t>& ends,
        const std::vector<int64_t>& steps, const std::vector<int64_t>& axes, const std::vector<int64_t>& decrease_axes = {}, const std::vector<int64_t>& none_axes = {});
template<typename T, typename TID = int> DLL_EXPORT int nonzero_count(Context* ctx, const T* x, TID* y, int64_t len, TID* workspace = nullptr);
template<typename T, typename TID = int64_t, typename TMID = int> DLL_EXPORT int nonzero_compute(Context* ctx, const T* x, TID* y, std::vector<int64_t>& xshape, int64_t nonzero_count, TMID* workspace = nullptr);
template<typename T, typename TID> DLL_EXPORT int unique_count(Context* ctx, const T* x, int64_t* unique_len,
        int64_t x_len, T* sorted_x = nullptr, TID* sorted_idx = nullptr, T* stage1_y = nullptr,
        TID* stage1_idx = nullptr, int64_t* count_min_max = nullptr, bool is_ascend_sorted = false);
template<typename T, typename TID> DLL_EXPORT int unique_compute(Context* ctx, const T* x, T* y, int64_t x_len,
        int64_t y_len, TID* idx, TID* count, TID* inverse, T* sorted_x = nullptr, TID* sorted_idx = nullptr,
        T* stage1_y = nullptr, TID* stage1_idx = nullptr, int64_t* count_min_max = nullptr,
        bool is_ascend_sorted = false);
template<typename T, typename TID> DLL_EXPORT int unique_dim_compute(Context* ctx, const T* x, T* y,
        const std::vector<int64_t>& xshape, std::vector<int64_t>& yshape, int64_t dim, TID* inverse_indices, TID* counts, bool sorted=false);
template<typename TX, typename TY = int64_t> DLL_EXPORT int where(Context* ctx, const TX* x, TY* y, const std::vector<int64_t>& xshape,
        int64_t nonzero_size);
template<typename T> DLL_EXPORT int masked_fill_scalar(Context* ctx, const bool* condition, const T* x, const T y, T* z,
        const std::vector<int64_t>& condition_shape, const std::vector<int64_t>& xshape);
template<typename T> DLL_EXPORT int select(Context* ctx, const bool* condition, const T* x, const T* y, T* z,
        const std::vector<int64_t>& condition_shape, const std::vector<int64_t>& xshape);
template<typename T> DLL_EXPORT int select_grad(Context* ctx, const bool* condition, const T* grad,
        T* dx, T* dy, const std::vector<int64_t>& condition_dims, const std::vector<int64_t>& output_dims);
template<typename T> DLL_EXPORT int masked_select(Context* ctx, const T* x, const bool* mask, T* y,
        const std::vector<int64_t>& x_shape, const std::vector<int64_t>& mask_shape, int64_t true_count);
template<typename T> DLL_EXPORT int masked_select_update(Context* ctx, const T* x, const bool* mask, T* y,
        const std::vector<int64_t>& y_shape, const std::vector<int64_t>& mask_shape, int64_t true_count, bool accumulate = false, bool is_scalar = false);
template<typename T> DLL_EXPORT int masked_select_grad(Context* ctx, const T* y, const bool* mask, T* x,
        const std::vector<int64_t>& x_shape, const std::vector<int64_t>& mask_shape, int64_t true_count);
template<typename T, typename TID> DLL_EXPORT int mask_label_by_index(Context* ctx, const T* logit, const TID* label, T* pred_logits,
            int64_t start_index, int64_t end_index, const int64_t N, const int64_t D, const int64_t nranks);
template<typename T, typename TID> DLL_EXPORT int mask_label_by_index_grad(Context* ctx, const T* dloss, const TID* label, T* dlogits,
            int64_t start_index, int64_t end_index, const int64_t N, const int64_t D);
template<typename T, typename TID> DLL_EXPORT int in_topk(Context* ctx, const T* x, const TID* y, bool* z,
        int64_t m, int64_t n, int64_t k);
template<typename T, typename TV = int> DLL_EXPORT int left_shift(Context* ctx, T* x, T* y, TV* val, int64_t len);
template<typename T, typename TV = int> DLL_EXPORT int right_shift(Context* ctx, T* x, T* y, TV* val, int64_t len);
template<typename T, typename TID> DLL_EXPORT int scatter(Context* ctx, const T* x,
        T* y, const VectorParam<TID>& indices, int64_t dim0, int64_t dim1, bool is_overwrite);
template<typename T, typename TID> DLL_EXPORT int scatter(Context* ctx, const T* x, const T* updates, T* y,
        const VectorParam<TID>& index, const std::vector<int64_t>& xshape, int64_t axis, bool is_overwrite);
template<typename T, typename TID> DLL_EXPORT int scatter_element(Context* ctx, const T* x, const T* updates,
        const TID* index, T* y, const std::vector<int64_t>& xyshape, const std::vector<int64_t>& updshape,
        const std::vector<int64_t>& idxshape, int64_t axis, int64_t reduction = 0);
template<typename T, typename TID> DLL_EXPORT int scatter_nd(Context* ctx, const T* x, const T* updates, T* y,
        const VectorParam<TID>& index, const VectorParam<int64_t>& xshape, const std::vector<int64_t>& index_shape,
        bool is_overwrite);
template<typename T, typename TID> DLL_EXPORT int scatter_grad(Context* ctx, const T* dy, const VectorParam<TID>& index,
        T* dx, T* dupdates, const std::vector<int64_t>& xshape, bool overwrite);
template<typename T, typename TID> DLL_EXPORT int index_pick(Context* ctx, const T* x, const TID* index, T* y,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& index_shape);
template<typename T, typename TID> DLL_EXPORT int index_put(Context* ctx, const T* x, const T* updates, const TID* index, T* y,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& index_shape, bool accumulate = false);
template<typename T> DLL_EXPORT int flip(Context* ctx, const T* x, T* y,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& axis);
template<typename T> DLL_EXPORT int unbind(Context* ctx, const T* x, std::vector<T*>& y,
        const std::vector<int64_t>& xshape, int64_t axis = 0);
template<typename T> DLL_EXPORT int permute_int8_weight_only(Context* ctx, const T* weight, T* permute_weight, int64_t n, int64_t k);
// sorting
template<typename T, typename TID> DLL_EXPORT int sort(Context* ctx, const T* x, T* y, TID* index, int64_t m, int64_t n,
        bool compare = false);
template<typename T, typename TID> DLL_EXPORT int sort_grad(Context* ctx,  const T* dy, const TID* index,
        T* dx, TID m, TID n);
template<typename T, typename TID = int> DLL_EXPORT int sorted_topk(Context* ctx, const T* x, T* y, TID* index,
        int64_t m, int64_t n, int64_t k, bool largest = true, bool precision = false);
template<typename T, typename TID = int> DLL_EXPORT int sorted_topk_with_filter(Context* ctx, const T* x, T* y,
        TID* index, int64_t m, int64_t n, int64_t k, bool largest, const float x_threshold, int64_t& k_filtered);
template<typename TX, typename TY = int64_t> DLL_EXPORT int argmax(Context* ctx, const TX* x, TY* y,
        const std::vector<int64_t>& xshape, int64_t axis);
template<typename TX, typename TY = int64_t> DLL_EXPORT int argmin(Context* ctx, const TX* x, TY* y,
        const std::vector<int64_t>& xshape, int64_t axis);
template<typename T, typename TID> DLL_EXPORT int search_sorted(Context* ctx, const T* x, const T* values, TID* y,
        int64_t m, int64_t xn, int64_t yn, bool is_rightside, bool is_ascend);
template<typename T, typename TID> DLL_EXPORT int stable_sort(Context* ctx, const T* x, T* y, TID* index, int64_t m, int64_t n,
        bool compare = false);
// rand generator
template<typename T> DLL_EXPORT int random(Context* ctx, T* x, int64_t len, T min, T max, int64_t seed);
template<typename T> DLL_EXPORT int normal(Context* ctx, T* x, T mean, T std, int64_t len, int64_t seed);
// broadcast ops
template<typename T> DLL_EXPORT int broadcast_add(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_sub(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_mul(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_div(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_max(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_min(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_pow(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_mod(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_floordiv(Context* ctx, const T* x, const T* y, T* z,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_equal(Context* ctx, const T* x, const T* y, bool* z,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_greater_than(Context* ctx, const T* x, const T* y, bool* z,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_greater_equal(Context* ctx, const T* x, const T* y, bool* z,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_less_than(Context* ctx, const T* x, const T* y, bool* z,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_less_equal(Context* ctx, const T* x, const T* y, bool* z,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_not_equal(Context* ctx, const T* x, const T* y, bool* z,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template <typename T> DLL_EXPORT int broadcast_addcdiv(Context* ctx, const T* w, const T* x, const T* y, T* z,
        const std::vector<int64_t>& wshape, const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape, float scalar);

// broadcast_grad ops
template<typename T> DLL_EXPORT int broadcast_add_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_sub_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_mul_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_div_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_max_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
template<typename T> DLL_EXPORT int broadcast_min_grad(Context* ctx, const T* x, const T* y, const T* z,
        const T* dz, T* dy, T* dx, const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape);
// fast Fourier transform
template<typename T> DLL_EXPORT int fft(Context* ctx, int64_t batch_size, int64_t N,
        const T* x_r, const T* x_i, T* y_r, T* y_i);
template<typename T> DLL_EXPORT int ifft(Context* ctx, int64_t batch_size, int64_t N,
        const T* x_r, const T* x_i, T* y_r, T* y_i);
template<typename T> DLL_EXPORT int fft2d(Context* ctx, const std::vector<int64_t>& shape,
        const T* x_r, const T* x_i, T* y_r, T* y_i);
template<typename T> DLL_EXPORT int fft3d(Context* ctx, const std::vector<int64_t>& shape,
        const T* x_r, const T* x_i, T* y_r, T* y_i);
template<typename T> DLL_EXPORT int sum(Context* ctx, const std::vector<const T*>& x_list, T* y, int64_t len);
template<typename T> DLL_EXPORT int logsumexp(Context* ctx, const T* x, T* y,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& axis);
template<typename T> DLL_EXPORT int logsumexp_grad(Context* ctx, const T* x, const T* y, const T* dy,
        T* dx, const std::vector<int64_t>& xshape, const std::vector<int64_t>& axis_shape);
//matrix
template<typename T> DLL_EXPORT int tril(Context* ctx, const T* x, T* y,
        const std::vector<int64_t>& xshape, int64_t diagonal);
template<typename T> DLL_EXPORT int triu(Context* ctx, const T* x, T* y,
        const std::vector<int64_t>& xshape, int64_t diagonal);
template<typename T> DLL_EXPORT int rbf_kernel(Context* ctx, const T* x, T* y, int64_t len, float mu, float sigma);
template<typename T> DLL_EXPORT int roll(Context* ctx, const T* x, T* y,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& shifts, const std::vector<int64_t>& axis);
template<typename T, typename TID = int> DLL_EXPORT int inverse(Context* ctx, const T* x, T* y, TID* info,
        int64_t batch, int64_t n);
template<typename T> DLL_EXPORT int roll_grad(Context* ctx, const T* x, const T* out_grad, T* x_grad,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& shifts, const std::vector<int64_t>& axis);
template<typename T> DLL_EXPORT int diag(Context* ctx, const T* x, T* y,
        const std::vector<int64_t>& xshape, const std::vector<int64_t>& yshape, int64_t offset = 0, T padvalue = 0);
template<typename T> DLL_EXPORT int einsum(Context* ctx, const std::vector<const T*>& x_list, T* y,
        const std::vector<std::vector<int64_t>>& xshape_list, const std::vector<int64_t>& y_shape, const char* equation);

// distribution
template<typename TP, typename TY> DLL_EXPORT int bernoulli(Context* ctx, const TP* p, TY* y, int64_t p_len, int64_t y_len, int64_t seed);
template <typename T> DLL_EXPORT int exponential(Context* ctx, const T lambda, T* y, int64_t y_len, int64_t seed);
template<typename T> DLL_EXPORT int adjacent_difference(Context* ctx, const T*x, T* y, int64_t len, int mode = 0, int64_t start = 0, int64_t end = -1);
template <typename T, typename TID> DLL_EXPORT int multinomial(Context* ctx, T* x, TID* y, int64_t num_samples,
        int64_t num_categories, int64_t num_distributions, bool replacement, int64_t seed);
template <typename T, typename TID = int> DLL_EXPORT int histc(Context* ctx, const T* x, TID* y, int64_t xlen, int64_t bins, T min_val, T max_val);
template<typename T> DLL_EXPORT int nan_to_num(Context* ctx, const T* src, T* dst, int64_t len, T nan, T pos_inf, T neg_inf);
}
}
}
#endif
