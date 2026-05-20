#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_LOSS_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_LOSS_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
namespace baidu {
namespace xpu {
namespace api {

// Loss and entropy
template<typename T> DLL_EXPORT int bce_loss(Context* ctx, const T* x, const T* label, T* y, int64_t len);
template<typename T> DLL_EXPORT int bce_loss_grad(Context* ctx, const T* x, const T* label, const T* dy, T* dx,
        int64_t len);
template<typename T> DLL_EXPORT int bce_with_logits_loss(Context* ctx, const T* x, const T* label, T* y, int64_t len);
template<typename T> DLL_EXPORT int bce_with_logits_loss_grad(Context* ctx, const T* x, const T* label, const T* dy, T* dx,
        int64_t len);
template<typename T> DLL_EXPORT int cos_sim(Context* ctx, const T* x, const T* y, T* z, int64_t xm, int64_t ym, int64_t n,
        T* x_norm, T* y_norm);
template<typename T> DLL_EXPORT int cos_sim_grad(Context* ctx, const T* x, const T* y, const T* z, const T* dz,
        T* dx, T* dy, int64_t xm, int64_t ym, int64_t n, const T* x_norm, const T* y_norm);
template<typename T> DLL_EXPORT int cosin_embedding_loss(Context* ctx, const T* x0, const T* x1, const T* labels, T* y,
        int64_t m, int64_t n, float margin = 0.0f);
template <typename T> DLL_EXPORT int cosin_embedding_loss_grad(Context* ctx, const T* x0, const T* x1, const T* labels,
        const T* y, const T* dy, const T* x0_norm, const T* x1_norm,
        T* dx0, T* dx1, int64_t m, int64_t n, float margin = 0.0f);
template<typename T, typename TID> DLL_EXPORT int ctc_loss(Context* ctx, const T* x,
        const int* labels, T* y, T* grad, const TID* seq_len, const TID* label_len,
        int64_t max_seq_length, int64_t batch, int64_t dim, int64_t max_label_length, int64_t blank = 0);
template<typename T, typename TID> DLL_EXPORT int ctc_loss_grad(Context* ctx, const T* dy, T* dx, const T* ctc_grad,
        int64_t max_seq_length, int64_t batch, int64_t dim, const TID* seq_len, bool norm_by_times);
template<typename T> DLL_EXPORT int gaussian_nll_loss(Context* ctx, const T* x, const T* var, const T* labels, T* y,
        int64_t len, bool full, float eps = 1e-6f);
template<typename T> DLL_EXPORT int gaussian_nll_loss_grad(Context* ctx, const T* x, const T* var, const T* labels, const T* dy,
        T* dx, T* dvar, int64_t len, float eps = 1e-6f);
template<typename T, typename TID> DLL_EXPORT int hard_cross_entropy(Context* ctx, const T* x, const TID* label,
        T* y, T* match_x, int64_t m, int64_t n, int64_t ignore_index);
template<typename T, typename TID> DLL_EXPORT int hard_cross_entropy_grad(Context* ctx, const TID* label,
        const T* match_x, const T* dy, T* dx, int64_t m, int64_t n, int64_t ignore_index);
template<typename T, typename TID> DLL_EXPORT int hard_softmax_with_cross_entropy(Context* ctx, const T* x,
        const TID* label, T* softmax, T* y, int64_t ignore_index, int64_t m, int64_t n);
template<typename T, typename TID> DLL_EXPORT int hard_softmax_with_cross_entropy_grad(Context* ctx, const T* dy,
        const TID* label, const T* softmax, T* dx, int64_t ignore_index, bool softmax_switch, int64_t m, int64_t n);
template<typename T> DLL_EXPORT int hinge_embedding_loss(Context* ctx, const T* x, const T* labels, T* y,
        int64_t len, float margin = 1.0f);
template<typename T> DLL_EXPORT int hinge_embedding_loss_grad(Context* ctx, const T* x, const T* labels, const T* dy, T* dx,
        int64_t len, float margin = 1.0f);
template<typename T> DLL_EXPORT int huber_loss(Context* ctx, const T* x, const T* y, T* diff, T* out,
        int64_t batch_size, int64_t pos_num, float delta);
template<typename T> DLL_EXPORT int huber_loss_grad(Context* ctx, const T* diff, const T* dout, T* dx, T* dy,
        int64_t batch_size, int64_t pos_num, float delta);
template<typename T> DLL_EXPORT int kldiv_loss(Context* ctx, const T* x, const T* labels, T* y, int64_t len,
        bool log_label = false);
template<typename T> DLL_EXPORT int kldiv_loss_grad(Context* ctx, const T* labels, const T* dy, T* dx,
        int64_t len, bool log_label = false);
template<typename T> DLL_EXPORT int l1_loss(Context* ctx, const T* x, const T* labels, T* y, int64_t len);
template<typename T> DLL_EXPORT int l1_loss_grad(Context* ctx, const T* x, const T* labels, const T* dy, T* dx,
        int64_t len);
template<typename T> DLL_EXPORT int log_loss(Context* ctx, const T* predict, const T* labels,
        T* loss, int64_t batch, float epsilon);
template<typename T> DLL_EXPORT int log_loss_grad(Context* ctx, const T* predict, const T* labels,
        const T* dloss, T* dpredict,  int64_t batch, float epsilon);
template<typename T> DLL_EXPORT int margin_ranking_loss(Context* ctx, const T* x0, const T* x1, const T* labels, T* y,
        int64_t len, float margin = 0.0f);
template<typename T> DLL_EXPORT int margin_ranking_loss_grad(Context* ctx, const T* x0, const T* x1, const T* labels, const T* dy,
        T* dx0, T* dx1, int64_t len, float margin = 0.0f);
template<typename T> DLL_EXPORT int mse_loss(Context* ctx, const T* x, const T* labels, T* y, int64_t len, int64_t reduction = 1);
template<typename T> DLL_EXPORT int mse_loss_grad(Context* ctx, const T* x, const T* labels, const T* dy, T* dx,
        int64_t len, int64_t reduction = 1);
template<typename T> DLL_EXPORT int multi_label_soft_margin_loss(Context* ctx, const T* x, const T* labels, T* y,
        int64_t m, int64_t n);
template<typename T> DLL_EXPORT int multi_label_soft_margin_loss_grad(Context* ctx, const T* x, const T* labels, const T* dy,
        T* dx, int64_t m, int64_t n);
template<typename T> DLL_EXPORT int multi_label_margin_loss(Context* ctx, const T* x, const T* labels, T* y,
        int64_t m, int64_t n);
template<typename T> DLL_EXPORT int multi_label_margin_loss_grad(Context* ctx, const T* x, const T* labels, const T* dy,
        T* dx, int64_t m, int64_t n);
template<typename T> DLL_EXPORT int multi_margin_loss(Context* ctx, const T* x, const T* labels, T* y,
        int64_t m, int64_t n, int64_t p = 1, float margin = 1.0f);
template<typename T> DLL_EXPORT int multi_margin_loss_grad(Context* ctx, const T* x, const T* labels, const T* dy,
        T* dx, int64_t m, int64_t n, int64_t p = 1, float margin = 1.0f);
template<typename T, typename TID = int> DLL_EXPORT int nll_loss(Context* ctx, const T* x, T* y, T* total_weight,
        const std::vector<int64_t>& x_shape, const TID* target, const T* weight = NULL,
        int64_t reduction = 1, int64_t ignore_index = -100);
template<typename T, typename TID = int> DLL_EXPORT int nll_loss_grad(Context* ctx, const T* dy, T* dx,
        const std::vector<int64_t>& shape, const TID* target, const T* weight = NULL,
        int64_t reduction = 1, int64_t ignore_index = -100, const T* total_weight = NULL);
template<typename T> DLL_EXPORT int poisson_nll_loss(Context* ctx, const T* x, const T* labels, T* y, int64_t len,
        bool log_input, bool full, float eps = 1e-8f);
template<typename T> DLL_EXPORT int poisson_nll_loss_grad(Context* ctx, const T* x, const T* labels, const T* dy, T* dx,
        int64_t len, bool log_input, float eps = 1e-8f);
template<typename T, typename TH = int> DLL_EXPORT int sigmoid_cross_entropy_with_logits(Context* ctx,
        const T* x, const T* label, T* y, int64_t m, int64_t n, TH* hit = nullptr, int64_t ignore_index = -100, const T* pos_weight = nullptr);
template<typename T, typename TH = int> DLL_EXPORT int sigmoid_cross_entropy_with_logits_grad(Context* ctx,
        const T* x, const T* label, const T* dy, T* dx, int64_t m, int64_t n, TH* hit = nullptr, int64_t ignore_index = -100, const T* pos_weight = nullptr);
template<typename T> DLL_EXPORT int smooth_l1_loss(Context* ctx, const T* x, const T* y, T* diff, T* out,
        bool has_weights, const T* inside_weights, const T* outside_weights, int64_t batch_size, int64_t pos_num, float sigma);
template<typename T> DLL_EXPORT int smooth_l1_loss_grad(Context* ctx, T* dx, T* dy, const T* diff, const T* dout,
        bool has_weights, T* inside_weights, T* outside_weights, int64_t batch_size, int64_t pos_num, float sigma);
template<typename T> DLL_EXPORT int soft_cross_entropy(Context* ctx, const T* x, const T* label, T* y, int64_t m, int64_t n);
template<typename T> DLL_EXPORT int soft_cross_entropy_grad(Context* ctx, const T* x, const T* label, const T* dy,
        T* dx, int64_t m, int64_t n);
template<typename T> DLL_EXPORT int soft_margin_loss(Context* ctx, const T* x, const T* labels, T* y,
        int64_t len);
template<typename T> DLL_EXPORT int soft_margin_loss_grad(Context* ctx, const T* x, const T* labels, const T* dy,
        T* dx, int64_t len);
template<typename T> DLL_EXPORT int soft_softmax_with_cross_entropy(Context* ctx, const T* x, const T* label,
        T* softmax, T* y, int64_t m, int64_t n);
template<typename T> DLL_EXPORT int soft_softmax_with_cross_entropy_grad(Context* ctx, const T* dy, const T* label,
        const T* softmax, T* dx, bool softmax_switch, int64_t m, int64_t n);
template<typename T> DLL_EXPORT int triplet_margin_loss(Context* ctx, const T* x0, const T* x1, const T* x2, T* y,
        int64_t m, int64_t n, int64_t p = 2, float margin = 1.0f, bool swap = false);
template<typename T> DLL_EXPORT int triplet_margin_loss_grad(Context* ctx, const T* x0, const T* x1, const T* x2, const T* dy,
        const T* d0, const T* d1, const T* d2, T* dx0, T* dx1, T* dx2,
        int64_t m, int64_t n, int64_t p = 2, float margin = 1.0f, bool swap = false);

}
}
}
#endif
