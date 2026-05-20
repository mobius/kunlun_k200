#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_BOX_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_BOX_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
namespace baidu {
namespace xpu {
namespace api {

template<typename T> DLL_EXPORT int box_area(Context* ctx, const T* boxes, T* area, int64_t n);
template<typename T> DLL_EXPORT int box_convert(Context* ctx, const T* in_boxes, T* out_boxes, int64_t n, int64_t in_fmt, int64_t out_fmt);
template<typename T> DLL_EXPORT int box_iou(Context* ctx, const T* boxes1, const T* boxes2, T* iou, int64_t m, int64_t n);
template<typename T> DLL_EXPORT int clip_box_to_image(Context* ctx, const T* boxes, T* clipped_boxes, int64_t n_boxes,
        int64_t h, int64_t w, bool pixel_offset = true);
template<typename T> DLL_EXPORT int complete_box_iou(Context* ctx, const T* boxes1, const T* boxes2,
        T* ciou, int64_t m, int64_t n, float eps = 1e-7);
template<typename T> DLL_EXPORT int distance_box_iou(Context* ctx, const T* boxes1, const T* boxes2,
        T* diou, int64_t m, int64_t n, float eps = 1e-7);
template<typename T> DLL_EXPORT int generalized_box_iou(Context* ctx, const T* boxes1, const T* boxes2,
        T* diou, int64_t m, int64_t n);
template<typename T, typename TID = int> DLL_EXPORT int remove_small_boxes(Context* ctx, const T* boxes,
        const T* im_info, TID* index, TID* n_keep, int64_t n_boxes, float min_size, bool is_scale = true,
        bool pixel_offset = true);
template<typename T> DLL_EXPORT int masks_to_boxes(Context* ctx, const T* masks, float* boxes,
        int64_t n, int64_t h, int64_t w);

}
}
}
#endif
