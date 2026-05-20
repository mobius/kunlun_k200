#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_CUSTOMIZATION_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_NN_CUSTOMIZATION_H
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
namespace baidu {
namespace xpu {
namespace api {

template<typename T> DLL_EXPORT int dfmb_psroi_align(Context* ctx,
        const T* x, T* y, const float* rois, int xn, int xc, int xh, int xw, int yn, int yc, int yh, int yw);
template<typename T> DLL_EXPORT int apollo_roi_pool(Context* ctx,
        const T* x, T* y, int* y_argmax, const float* rois, int xn, int xc, int xh, int xw, int yn, int yc, int yh, int yw);
template<typename T> DLL_EXPORT int rcnn_proposal(Context* ctx,
        const T* cls_score_softmax, const T* bbox_pred, const T* rois, const T* im_info, T* result_boxes, int batch_size,
        const std::vector<float>& bbox_mean_, const std::vector<float>& bbox_std_, const std::vector<float>& thresholds_,
        float min_size_h_, float min_size_w_, float threshold_objectness_, float overlap_ratio_,
        int num_class_, int num_rois_, int max_candidate_n_, int min_size_mode_, int top_n_,
        int out_channel_, int acc_box_num_, bool refine_out_of_map_bbox_ = true,
        bool regress_agnostic_ = false, bool rpn_proposal_output_score_ = true);
template<typename T> DLL_EXPORT int rpn_proposal_ssd(Context* ctx,
        const T* rpn_cls_prob_reshape, const T* rpn_bbox_pred, const T* im_info, const T* anchor_heights_,
        const T* anchor_widths_, T* out_rois,
        int batchSize, int num_anchor_per_point_, int height_, int width_, const std::vector<T>& bbox_mean_,
        const std::vector<T>& bbox_std_, int top_n_,
        int heat_map_a_, bool refine_out_of_map_bbox_, float threshold_objectness_, int min_size_mode_, float min_size_h_,
        float min_size_w_,
        int max_candidate_n_, float overlap_ratio_, int* out_rois_num_);

template<typename T> DLL_EXPORT int apollo_yolo_get_object(Context* ctx,
        int n, const T* loc_data, const T* obj_data, const T* cls_data, const T* ori_data, const T* dim_data,
        const T* lof_data, const T* lor_data, const T* area_id_data,
        const T* visible_ratio_data, const T* cut_off_ratio_data,
        const T* brvis_data, const T* brswt_data, const T* ltvis_data,
        const T* ltswt_data, const T* rtvis_data, const T* rtswt_data,
        const T* anchor_data, const T* expand_data, int width, int height,
        int num_anchors, int num_classes, float confidence_threshold,
        float light_vis_conf_threshold, float light_swt_conf_threshold,
        bool with_box3d, bool with_frbox, bool with_lights, bool with_ratios,
        bool multi_scale, int num_areas, T* res_box_data, T* res_cls_data,
        int res_cls_offset, int all_scales_num_candidates);
template<typename T> DLL_EXPORT int apply_nms(Context* ctx, const T* bbox_data, const T* conf_data,
        const std::vector<int>& origin_indices, const int bbox_step,
        const float confidence_threshold, const int top_k,
        const float nms_threshold, std::vector<int>& indices,
        bool* overlapped, int* idx_sm);
template<typename T> DLL_EXPORT int build_nodes(Context* ctx, const T* offset_map, const T* prob_map,
        uint32_t* center_node, uint16_t* status, int start_row_index, int end_row_index, int rows, int cols,
        float scale, float objectness_threshold);
template<typename T> DLL_EXPORT int build_nodes(Context* ctx, const T* offset_map, const T* prob_map,
        void* nodes, int start_row_index, int end_row_index, int rows, int cols,
        float scale, float objectness_threshold);
template<typename T> DLL_EXPORT int feature_generator(Context* ctx, const T* pc, int* point2grid, T* log_table,
        T* top_intensity_data, T* max_height_data, T* mean_height_data,
        T* mean_intensity_data, T* count_data, T* nonempty_data,
        const int cloud_size, const int map_size, const int max_log_num, bool use_intensity_feature);
template<typename T> DLL_EXPORT int wenxin_secure(Context* ctx, const T* x, T* y, int h, int w, int64_t num, int64_t key, int encrypted_len);
template<typename T, typename TID = int> DLL_EXPORT int top_p_sampling(Context* ctx, const T* x, const T* top_ps,
        const float* rand_coeff, TID* top_p_ids, int64_t bs, int64_t vocab_size, T* top_p_vales = nullptr, const bool* stop_flags = nullptr,
        const int k = 0, TID* top_k_ids = nullptr, T* top_k_values = nullptr);
template<typename T, typename TID = int> DLL_EXPORT int faster_top_p_sampling(Context* ctx, const T* x, const T* top_ps,
        const float* rand_coeff, TID* top_p_ids, int64_t bs, int64_t vocab_size, T* top_p_vales = nullptr, const bool* stop_flags = nullptr,
        const int fast_valus = 20, const int k = 0, TID* top_k_ids = nullptr, T* top_k_values = nullptr);
template<typename T> DLL_EXPORT int token_repetition_penalty(Context* ctx, const int64_t* pre_ids, const T* penalty_scores, const T* logits, T* y,
        const int64_t bs, const int64_t length, const int64_t length_id);
}
}
}
#endif

