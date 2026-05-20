#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_ATTENTION_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_GTEST_ATTENTION_H
#include "xpu/xdnn.h"
#include "xpu/refactor/gtest/xdnn_gtest.h"

static std::vector<int> get_attn_lod_vec(const std::initializer_list<int64_t> lod_vec_list){
    std::vector<int> res(lod_vec_list.begin(), lod_vec_list.end());
    return res;
}
// make me more elegant, using same input param, and deduce return type
static api::QKVAttnParam create_attn_param(bool is_vsl, int64_t batch, int64_t max_seqlen, 
        int64_t head_num, int64_t head_dim, api::Activation_t act, float alpha,
        int64_t last_slice, bool do_fc_qkv_fusion, int64_t hidden_dim, int64_t pad_seqlen, std::vector<int>&& lod,
        int64_t mask_ld, const std::initializer_list<int64_t>& zshape, std::vector<int>&& sqlod_vec, 
        int64_t qkv_shape = 0, int64_t _max_ptr_type = 1, int64_t ldz = -1,
        bool is_pre_norm = false, bool is_perchannel = false,
        int64_t relative_type = 0, int64_t max_pos_len = 512,
        bool is_smooth_quant = false, bool mask_autogen = false) {
    AttnMacMaxPtrType_t max_ptr_type = (AttnMacMaxPtrType_t) static_cast<int>(_max_ptr_type);
    if (is_vsl) {
        api::VectorParam<int> query_lod = {lod.data(), (int)lod.size(), nullptr};
        api::VectorParam<int> sqlod = {sqlod_vec.data(), (int) sqlod_vec.size(), nullptr};
        api::QKVAttnParam p(query_lod, head_num, head_dim, act, last_slice, do_fc_qkv_fusion, pad_seqlen, hidden_dim, 
                        is_pre_norm, is_perchannel, qkv_shape, zshape, max_ptr_type, ldz, sqlod, alpha);
        p.triangle_mask_autogen = mask_autogen;
        return p;
    } else {
        api::QKVAttnParam p(batch, max_seqlen, head_num, head_dim, zshape,
                        act, last_slice, do_fc_qkv_fusion, hidden_dim, is_pre_norm, 
                        is_perchannel, qkv_shape, max_ptr_type, ldz, alpha);
        p.triangle_mask_autogen = mask_autogen;
        return p;
    }
}

static api::QKVAttnParam create_attn_param(bool is_vsl, int64_t batch, int64_t max_seqlen,
        std::array<int64_t, 2> head_num_info, int64_t head_dim, api::Activation_t act, float alpha,
        int64_t last_slice, bool do_fc_qkv_fusion, int64_t hidden_dim, int64_t pad_seqlen, std::vector<int>&& lod,
        int64_t mask_ld, const std::initializer_list<int64_t>& zshape, std::vector<int>&& sqlod_vec,
        int64_t qkv_shape = 0, int64_t _max_ptr_type = 1, int64_t ldz = -1,
        bool is_pre_norm = false, bool is_perchannel = false,
        int64_t relative_type = 0, int64_t max_pos_len = 512,
        bool is_smooth_quant = false, bool mask_autogen = false) {
    AttnMacMaxPtrType_t max_ptr_type = (AttnMacMaxPtrType_t) static_cast<int>(_max_ptr_type);
    if (is_vsl) {
        api::VectorParam<int> query_lod = {lod.data(), (int)lod.size(), nullptr};
        api::VectorParam<int> sqlod = {sqlod_vec.data(), (int) sqlod_vec.size(), nullptr};
        api::QKVAttnParam p(query_lod, head_num_info[0], head_dim, act, last_slice, do_fc_qkv_fusion, pad_seqlen, hidden_dim,
                        is_pre_norm, is_perchannel, qkv_shape, zshape, max_ptr_type, ldz, sqlod, alpha);
        p.key_value_head_num = head_num_info[1];
        p.triangle_mask_autogen = mask_autogen;
        return p;
    } else {
        api::QKVAttnParam p(batch, max_seqlen, head_num_info[0], head_dim, zshape,
                        act, last_slice, do_fc_qkv_fusion, hidden_dim, is_pre_norm,
                        is_perchannel, qkv_shape, max_ptr_type, ldz, alpha);
        p.key_value_head_num = head_num_info[1];
        p.triangle_mask_autogen = mask_autogen;
        return p;
    }
}

static api::DifSeqAttnParam create_attn_param(bool is_vsl, int64_t batch, int64_t max_seqlen, int64_t head_num, 
        int64_t head_dim, int64_t max_qlen, int64_t max_klen, bool do_fc_qkv_fusion, 
        std::vector<int>&& lod, const std::initializer_list<int64_t>& zshape, int64_t do_softmax = 1, int64_t attn_probs_trans = 0) {
    if (is_vsl) {
        api::VectorParam<int> query_lod = {lod.data(), (int)lod.size(), nullptr};
        // api::QKVAttnParam p(query_lod, head_num, head_dim, act, last_slice, do_fc_qkv_fusion, pad_or_maskld, hidden_dim);
        api::DifSeqAttnParam p(query_lod, head_num, head_dim, do_softmax,
            attn_probs_trans, zshape, do_fc_qkv_fusion);
        return p;
    } else {
        api::DifSeqAttnParam p(batch, max_qlen, max_klen, head_num, head_dim,
            zshape, do_softmax, attn_probs_trans, do_fc_qkv_fusion);
        return p;
    } 
}

static api::DecodeAttnParam create_attn_param(int64_t batch, int64_t max_seqlen, int64_t head_num,
        int64_t head_dim, int64_t step, int64_t qkv_shape, int64_t vbatch, std::vector<int>&& vbmap,
        const std::vector<int64_t>& zshape = {}) {
    api::VectorParam<int> vbmap_xpu = {vbmap.data(), (int)vbmap.size(), nullptr};
    api::DecodeAttnParam p(batch, max_seqlen, head_num, head_dim, step, qkv_shape, vbatch, vbmap_xpu, zshape);
    return p;
}

static api::DecodeAttnParam create_attn_param(int64_t batch, int64_t max_seqlen,
        std::array<int64_t, 2> head_num_info, int64_t head_dim, int64_t step, int64_t qkv_shape, int64_t vbatch, std::vector<int>&& vbmap,
        const std::vector<int64_t>& zshape = {}) {
    api::VectorParam<int> vbmap_xpu = {vbmap.data(), (int)vbmap.size(), nullptr};
    api::DecodeAttnParam p(batch, max_seqlen, head_num_info[0], head_dim, step, qkv_shape, vbatch, vbmap_xpu, zshape);
    p.key_value_head_num = head_num_info[1];
    return p;
}

static api::DecodeAttnParam create_attn_param(int64_t batch, int64_t max_seqlen, int64_t head_num, 
        int64_t head_dim, std::vector<int>&& lod, int64_t qkv_shape, int64_t vbatch, std::vector<int>&& vbmap, const std::vector<int64_t>& zshape = {}) {
    api::VectorParam<int> query_lod = {lod.data(), (int)lod.size(), nullptr};
    api::VectorParam<int> vbmap_xpu = {vbmap.data(), (int)vbmap.size(), nullptr};
    api::DecodeAttnParam p(query_lod, max_seqlen, head_num, head_dim, -1, qkv_shape, vbatch, vbmap_xpu);
    return p;
}

static api::DecodeAttnParam create_attn_param(int64_t batch,
                                              int64_t max_seqlen,
                                              std::array<int64_t, 2> head_num_info,
                                              int64_t head_dim,
                                              std::vector<int>&& lod,
                                              int64_t qkv_shape,
                                              int64_t vbatch,
                                              std::vector<int>&& vbmap,
                                              const std::vector<int64_t>& zshape = {}) {
    api::VectorParam<int> query_lod = {lod.data(), (int)lod.size(), nullptr};
    api::VectorParam<int> vbmap_xpu = {vbmap.data(), (int)vbmap.size(), nullptr};
    api::DecodeAttnParam p(query_lod, max_seqlen, head_num_info[0], head_dim, -1, qkv_shape, vbatch, vbmap_xpu);
    p.key_value_head_num = head_num_info[1];
    return p;
}

static api::CrossAttnParam create_attn_param(bool is_vsl, int64_t batch, int64_t max_seqlen, int64_t head_num, 
        int64_t head_dim, float alpha, int64_t max_qlen, int64_t max_kvlen, bool do_fc_qkv_fusion, std::vector<int>&& lod, 
        const std::initializer_list<int64_t>& zshape, std::vector<int>&& vbmap, int64_t slice_conf = -1, int64_t qkv_shape = 0, int64_t do_softmax = 1, int64_t attn_probs_trans = 0,
        int64_t step = -1, int64_t vbatch = -1, bool mask_autogen = false) {
    api::VectorParam<int> vbmap_vp = {vbmap.data(), (int)vbmap.size(), nullptr};
    if (is_vsl) {
        api::VectorParam<int> query_lod = {lod.data(), (int)lod.size(), nullptr};
        // api::QKVAttnParam p(query_lod, head_num, head_dim, act, last_slice, do_fc_qkv_fusion, pad_or_maskld, hidden_dim);
        api::CrossAttnParam p(batch, query_lod, head_num, head_dim, do_fc_qkv_fusion, zshape, 
                slice_conf, qkv_shape, do_softmax, attn_probs_trans, step, vbatch, vbmap_vp, alpha);
        p.triangle_mask_autogen = mask_autogen;
        return p;
    } else {
        api::CrossAttnParam p(batch, max_qlen, head_num, head_dim, do_fc_qkv_fusion, max_qlen, max_kvlen,
                zshape, slice_conf, qkv_shape, do_softmax, attn_probs_trans, step, vbatch, vbmap_vp, alpha);
        p.triangle_mask_autogen = mask_autogen;
        return p;
    }
}

static api::CrossAttnParam create_attn_param(bool is_vsl, int64_t batch, int64_t max_seqlen, std::array<int64_t, 2> head_num_info, 
        int64_t head_dim, float alpha, int64_t max_qlen, int64_t max_kvlen, bool do_fc_qkv_fusion, std::vector<int>&& lod, 
        const std::initializer_list<int64_t>& zshape, std::vector<int>&& vbmap, int64_t slice_conf = -1, int64_t qkv_shape = 0, int64_t do_softmax = 1, int64_t attn_probs_trans = 0,
        int64_t step = -1, int64_t vbatch = -1, bool mask_autogen = false) {
    api::VectorParam<int> vbmap_vp = {vbmap.data(), (int)vbmap.size(), nullptr};
    if (is_vsl) {
        api::VectorParam<int> query_lod = {lod.data(), (int)lod.size(), nullptr};
        api::CrossAttnParam p(batch, query_lod, head_num_info[0], head_dim, do_fc_qkv_fusion, zshape, 
                slice_conf, qkv_shape, do_softmax, attn_probs_trans, step, vbatch, vbmap_vp, alpha);
        p.key_value_head_num = head_num_info[1];
        p.triangle_mask_autogen = mask_autogen;
        return p;
    } else {
        api::CrossAttnParam p(batch, max_qlen, head_num_info[0], head_dim, do_fc_qkv_fusion, max_qlen, max_kvlen,
                zshape, slice_conf, qkv_shape, do_softmax, attn_probs_trans, step, vbatch, vbmap_vp, alpha);
        p.key_value_head_num = head_num_info[1];
        p.triangle_mask_autogen = mask_autogen;
        return p;
    }

}

static api::PageAttnParam<int> create_page_param(int64_t block_size,
                                                 int64_t max_batch_size,
                                                 int64_t max_num_blocks_per_seq,
                                                 std::vector<int>&& real_batch,
                                                 int quant_type) {
    api::VectorParam<int> real_batch_lod = {real_batch.data(), (int)real_batch.size(), nullptr};
    api::PageAttnParam<int> p(block_size, max_batch_size, max_num_blocks_per_seq, real_batch_lod, quant_type);
    return p;
}

#endif
