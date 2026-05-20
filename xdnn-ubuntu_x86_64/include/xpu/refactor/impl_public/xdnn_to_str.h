#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_TO_STR_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XDNN_TO_STR_H

#include "xpu/refactor/attention.h"
#include "xpu/refactor/fusion_cv_param.h"
#include "xpu/refactor/context/newcontext.h"
#include "xpu/refactor/impl_public/xdnn_pointer.h"
#include "xpu/xdnn_types.h"
#include <sstream>
#include <string>
#include <vector>
namespace baidu {
namespace xpu {
namespace api {

///////////////////////////////// basic_type_to_str /////////////////////////////////////////
template <typename T> inline static std::string basic_type_to_str() { return "unknown"; }
template <> inline std::string basic_type_to_str<bool>() { return "bool"; }
template <> inline std::string basic_type_to_str<int8_t>() { return "int8_t"; }
template <> inline std::string basic_type_to_str<int16_t>() { return "int16_t"; }
template <> inline std::string basic_type_to_str<int>() { return "int"; }
template <> inline std::string basic_type_to_str<int64_t>() { return "int64_t"; }
template <> inline std::string basic_type_to_str<float>() { return "float"; }
template <> inline std::string basic_type_to_str<float16>() { return "float16"; }
template <> inline std::string basic_type_to_str<bfloat16>() { return "bfloat16"; }
template <> inline std::string basic_type_to_str<uint8_t>() { return "uint8_t"; }
template <> inline std::string basic_type_to_str<uint16_t>() { return "uint16_t"; }
template <> inline std::string basic_type_to_str<uint32_t>() { return "uint32_t"; }
template <> inline std::string basic_type_to_str<double>() { return "double"; }
template <> inline std::string basic_type_to_str<int_with_ll_t>() { return "int_with_ll_t"; }
template <> inline std::string basic_type_to_str<tfloat32>() { return "tfloat32"; }
template <> inline std::string basic_type_to_str<int8_wo_t>() { return "int8_wo_t"; }
template <> inline std::string basic_type_to_str<int15_wo_t>() { return "int15_wo_t"; }
template <> inline std::string basic_type_to_str<int4_t>() { return "int4_t"; }
template <> inline std::string basic_type_to_str<int4_wo_int8>() { return "int4_wo_int8"; }
template <> inline std::string basic_type_to_str<int4_wo_int15>() { return "int4_wo_int15"; }

template <int TYPE> inline static std::string basic_type_to_str() { return std::to_string(TYPE); }

///////////////////////////////// xdnn_param_to_str /////////////////////////////////////////
inline std::string xdnn_param_to_str(Context* ctx, std::string v) { return v; }
inline std::string xdnn_param_to_str(Context* ctx, const Activation_t& act) { return act.to_string(); }
inline std::string xdnn_param_to_str(Context* ctx, bool v) { return v ? "true" : "false"; }
template <typename T> inline std::string xdnn_param_to_str(Context* ctx, T v) { return std::to_string(v); }

template <> inline std::string xdnn_param_to_str<float>(Context* ctx, float v) {
    std::stringstream sstream;
    if (std::isinf(v)) {
        if (v > 0) {
            sstream << "INFINITY";
        } else {
            sstream << "-INFINITY";
        }
    } else if (std::isnan(v)) {
        sstream << "NAN";
    } else {
        sstream << v;
    }
    return std::string(sstream.str());
}

template <typename T> inline std::string xdnn_param_to_str(Context* ctx, T* ptr) {
    auto ktype = pointer_type(ctx, (const void*)ptr);
    if (ktype == kNIL) {
        return "\"NULL\"";
    } else if (ktype == kL3) {
        return "\"L3\"";
    } else if (ktype == kGM) {
        return "\"GM\"";
    } else if (ktype == kHOST) {
        return "\"CPU\"";
    } else {
        return "\"INVALID\"";
    }
}
template <typename T> inline std::string xdnn_param_to_str(Context* ctx, const T* ptr) {
    return xdnn_param_to_str(ctx, const_cast<T*>(ptr));
}
template <typename T> inline std::string xdnn_param_to_str(Context* ctx, VectorParam<T> vp) {
    std::string ret = "{";
    if (vp.cpu != nullptr) {
        if (vp.len > 0) {
            ret = ret + std::to_string(vp.cpu[0]);
        }
        for (int i = 1; i < vp.len; i++) {
            ret = ret + ", " + std::to_string(vp.cpu[i]);
        }
        ret = ret + "}";
    } else {
        // due to the cpu info is too long, use this branch
        // to support more in future
        ret = "\"cpu_nullptr\"";
    }
    return ret;
}
template <typename T> inline std::string xdnn_param_to_str(Context* ctx, const std::vector<T>& v) {
    std::string ret = "{";
    if (v.size() > 0) {
        ret = ret + xdnn_param_to_str(ctx, v[0]);
    }
    for (int i = 1; i < v.size(); i++) {
        ret = ret + ", " + xdnn_param_to_str(ctx, v[i]);
    }
    ret = ret + "}";
    return ret;
}

template <typename T> static inline std::string attn_vp_dump(Context* ctx, VectorParam<T> vp) {
    std::string ret = "get_attn_lod_vec({";
    if (vp.cpu != nullptr) {
        if (vp.len > 0) {
            ret = ret + std::to_string(vp.cpu[0]);
        }
        for (int i = 1; i < vp.len; i++) {
            ret = ret + ", " + std::to_string(vp.cpu[i]);
        }
        ret = ret + "})";
    } else {
        // to support more in future
        ret = ret + "})";
    }
    return ret;
}
// if vsl: dump to {true, batch, max_seqlen, head_num, head_dim, hidden_dim, act, alpha, last_slice_seq, do_fc_fusion,
// padseqlen, lod} if novsl: dump to {false, batch, max_seqlen, head_num, head_dim, hidden_dim, act, alpa,
// last_slice_seq, do_fc_fusion, padseqlen, mask_lod, mask_shape}
static inline std::string xdnn_param_to_str(Context* ctx, const QKVAttnParam& p) {
    std::string ret = "create_attn_param(";
    ret = ret + xdnn_param_to_str(ctx, p.is_vsl);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.batch);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.max_seqlen);
    if (p.head_num == p.key_value_head_num) {
        ret = ret + ", " + xdnn_param_to_str(ctx, p.head_num);
    } else {
        ret = ret + ", {" + xdnn_param_to_str(ctx, p.head_num);
        ret = ret + ", " + xdnn_param_to_str(ctx, p.key_value_head_num) + "}";
    }
    ret = ret + ", " + xdnn_param_to_str(ctx, p.head_dim);
    ret = ret + ", " + p.act.to_string();
    ret = ret + ", " + xdnn_param_to_str(ctx, p.alpha);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.last_slice_seq);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.do_fc_qkv_fusion);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.hidden_dim);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.pad_seqlen);
    ret = ret + ", " + attn_vp_dump(ctx, p.lod);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.mask_ld);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.zshape);
    // xpu1
    ret = ret + ", " + attn_vp_dump(ctx, p.sqlod);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.qkv_shape);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.max_ptr_type);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.ldz);
    // transformer
    ret = ret + ", " + xdnn_param_to_str(ctx, p.is_pre_norm);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.is_perchannel);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.relative_type);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.max_pos_len);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.is_smooth_quant);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.triangle_mask_autogen);
    ret = ret + ")";
    return ret;
}

static inline std::string xdnn_param_to_str(Context* ctx, const DifSeqAttnParam& p) {
    std::string ret = "create_attn_param(";
    ret = ret + xdnn_param_to_str(ctx, p.is_vsl);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.batch);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.max_seqlen);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.head_num);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.head_dim);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.max_q_seq);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.max_kv_seq);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.do_fc_qkv_fusion);
    ret = ret + ", " + attn_vp_dump(ctx, p.lod);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.zshape);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.do_softmax);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.attn_probs_trans);
    ret = ret + ")";
    return ret;
}

static inline std::string xdnn_param_to_str(Context* ctx, const DecodeAttnParam& p) {
    std::string ret = "create_attn_param(";
    ret = ret + xdnn_param_to_str(ctx, p.batch);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.max_seqlen);
    if (p.head_num == p.key_value_head_num) {
        ret = ret + ", " + xdnn_param_to_str(ctx, p.head_num);
    } else {
        ret = ret + ", {" + xdnn_param_to_str(ctx, p.head_num);
        ret = ret + ", " + xdnn_param_to_str(ctx, p.key_value_head_num) + "}";
    }
    ret = ret + ", " + xdnn_param_to_str(ctx, p.head_dim);
    if(p.is_vsl){
        ret = ret + ", " + attn_vp_dump(ctx, p.lod);
    } else {
        ret = ret + ", " + xdnn_param_to_str(ctx, p.step);
    }
    ret = ret + ", " + xdnn_param_to_str(ctx, p.qkv_shape);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.vbatch);
    ret = ret + ", " + attn_vp_dump(ctx, p.vb_map);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.zshape);
    ret = ret + ")";
    return ret;
}

static inline std::string xdnn_param_to_str(Context* ctx, const CrossAttnParam& p) {
    std::string ret = "create_attn_param(";
    ret = ret + xdnn_param_to_str(ctx, p.is_vsl);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.batch);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.max_seqlen);
    if (p.head_num == p.key_value_head_num) {
        ret = ret + ", " + xdnn_param_to_str(ctx, p.head_num);
    } else {
        ret = ret + ", {" + xdnn_param_to_str(ctx, p.head_num);
        ret = ret + ", " + xdnn_param_to_str(ctx, p.key_value_head_num) + "}";
    }
    ret = ret + ", " + xdnn_param_to_str(ctx, p.head_dim);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.alpha);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.max_q_seq);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.max_kv_seq);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.do_fc_qkv_fusion);
    ret = ret + ", " + attn_vp_dump(ctx, p.lod);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.zshape);
    ret = ret + ", " + attn_vp_dump(ctx, p.vb_map);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.slice_conf);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.qkv_shape);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.do_softmax);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.attn_probs_trans);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.step);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.vbatch);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.triangle_mask_autogen);
    ret = ret + ")";
    return ret;
}

static inline std::string xdnn_param_to_str(Context* ctx, const BaseAttnParam& p) {
    if (p.get_attn_type() == AttnType_t::QKV_ATTENTION) {
        return xdnn_param_to_str(ctx, dynamic_cast<const QKVAttnParam&>(const_cast<BaseAttnParam&>(p)));
    }
    if (p.get_attn_type() == AttnType_t::DIF_SEQ_ATTENTION) {
        return xdnn_param_to_str(ctx, dynamic_cast<const DifSeqAttnParam&>(const_cast<BaseAttnParam&>(p)));
    }
    if (p.get_attn_type() == AttnType_t::DECODE_ATTENTION) {
        return xdnn_param_to_str(ctx, dynamic_cast<const DecodeAttnParam&>(const_cast<BaseAttnParam&>(p)));
    }
    if (p.get_attn_type() == AttnType_t::CROSS_ATTENTION) {
        return xdnn_param_to_str(ctx, dynamic_cast<const CrossAttnParam&>(const_cast<BaseAttnParam&>(p)));
    }
    return "Attention not supported";
}

static inline std::string xdnn_param_to_str(Context* ctx, const CVFusionConv2dParam& p) {
    std::string ret = "api::CVFusionConv2dParam(";
    ret = ret + xdnn_param_to_str(ctx, p.f);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.c);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.group);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.ksize);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.stride);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.pad);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.dilation);
    ret = ret + ")";
    return ret;
}

static inline std::string xdnn_param_to_str(Context* ctx, const CVFusionBatchNormParam& p) {
    std::string ret = "api::CVFusionBatchNormParam(";
    ret = ret + xdnn_param_to_str(ctx, p.eps);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.momentum);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.scale);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.bias);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.batch_mean);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.batch_inv_std);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.global_mean);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.global_var);
    ret = ret + ")";
    return ret;
}

static inline std::string xdnn_param_to_str(Context* ctx, const CVFusionBatchNormGradParam& p) {
    std::string ret = "api::CVFusionBatchNormGradParam(";
    ret = ret + xdnn_param_to_str(ctx, p.scale);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.batch_mean);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.batch_inv_std);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.dscale);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.dbias);
    ret = ret + ")";
    return ret;
}

static inline std::string xdnn_param_to_str(Context* ctx, const PageAttnParam<int>& p) {
    std::string ret = "create_page_param(";
    ret = ret + xdnn_param_to_str(ctx, p.block_size);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.max_batch_size);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.max_num_blocks_per_seq);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.real_batch);
    ret = ret + ", " + xdnn_param_to_str(ctx, p.quant_type);
    ret = ret + ")";
    return ret;
}

} // namespace api
} // namespace xpu
} // namespace baidu
#endif
