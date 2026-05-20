#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CONTEXT_ATTENTION_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CONTEXT_ATTENTION_H

#include "xpu/dll_export.h"
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
#include <vector>
#ifdef _MSC_VER
#include <algorithm> // std::max, std::min
#endif
#include "xpu/refactor/page_attention.h" // for page attention

// some attention variants we will support
typedef enum {
    BASE_ATTENTION = 1,
    QKV_ATTENTION = 2,     // wrapper for old implementation
    DIF_SEQ_ATTENTION = 4, // common situation for Q's seqlen differs from K's seqlen
    DECODE_ATTENTION = 8,  // common situation for decoder, qslice, cal step but not max_seqlen. only novsl
    CROSS_ATTENTION = 9,
    USER_DEF_ATTENTION = 16, // user can define an attention variant by himself (only when our kernel support)
} AttnType_t;

typedef enum {
    ATTN_WHOLE_BATCH = 1,
    ATTN_PER_BATCH = 2, // in kl2, we use local quant to suport this
} AttnMacMaxPtrType_t;

namespace baidu {
namespace xpu {
namespace api {

template <typename TID = int> class DLL_EXPORT NewBaseAttnParam {
  public:
    const bool is_vsl;
    bool do_fc_qkv_fusion;
    int64_t batch;
    int64_t max_seqlen;
    const int64_t head_num;
    const int64_t head_dim;
    VectorParam<TID> lod;
    std::vector<int64_t> zshape; // normally, it means mask
    float alpha;
    int64_t key_value_head_num; // for GQA or MQA
    int64_t m_config = -1; // for stable test

  public:
    NewBaseAttnParam() = delete;
    // those were made for supporting int64_t
    NewBaseAttnParam(bool _do_fc_qkv_fusion, int64_t _batch, int64_t _head_num, int64_t _head_dim,
            VectorParam<TID> _lod, const std::vector<int64_t>& _zshape, float _alpha = 0.0f, int64_t _ldkv = -1)
        : is_vsl(true), do_fc_qkv_fusion(_do_fc_qkv_fusion), batch(_batch), head_num(_head_num), head_dim(_head_dim),
          lod(_lod) {
        alpha = _alpha == 0.0f ? 1.0f / std::sqrt(1.0f * head_dim) : _alpha;
        max_seqlen = -1;
        if (lod.cpu[0] == 0 && batch >= 1) {
            max_seqlen = lod.cpu[1];
            int64_t min_seqlen = lod.cpu[1];
            for (int64_t i = 1; i < batch; i++) {
                int64_t seqlen = lod.cpu[i + 1] - lod.cpu[i];
                max_seqlen = std::max<int64_t>(max_seqlen, seqlen);
                min_seqlen = std::min<int64_t>(min_seqlen, seqlen);
            }
            if (min_seqlen <= 0) {
                max_seqlen = -1;
            }
        }
        max_seqlen = std::max<int64_t>(max_seqlen, _ldkv);
        zshape.resize(_zshape.size(), 0);
        for (size_t i = 0; i < _zshape.size(); i++) {
            zshape[i] = _zshape[i];
        }
        this->key_value_head_num = _head_num; // default case is MHA
    };
    NewBaseAttnParam(bool _do_fc_qkv_fusion, int64_t _batch, int64_t _seqlen, int64_t _head_num, int64_t _head_dim,
            const std::vector<int64_t>& _zshape, float _alpha = 0.0f)
        : is_vsl(false), do_fc_qkv_fusion(_do_fc_qkv_fusion), batch(_batch), max_seqlen(_seqlen), head_num(_head_num),
          head_dim(_head_dim) {
        lod.len = 0;
        alpha = _alpha == 0.0f ? 1.0f / std::sqrt(1.0f * head_dim) : _alpha;
        zshape.resize(_zshape.size(), 0);
        for (size_t i = 0; i < _zshape.size(); i++) {
            zshape[i] = _zshape[i];
        }
        this->key_value_head_num = _head_num; // default case is MHA
    };
    NewBaseAttnParam(bool _do_fc_qkv_fusion, int64_t _batch, int64_t _head_num, int64_t _head_dim,
            VectorParam<TID> _lod, const std::initializer_list<int64_t>& _zshape)
        : NewBaseAttnParam(_do_fc_qkv_fusion, _batch, _head_num, _head_dim, _lod, std::vector<int64_t>(_zshape)){};
    NewBaseAttnParam(bool _do_fc_qkv_fusion, int64_t _batch, int64_t _seqlen, int64_t _head_num, int64_t _head_dim,
            const std::initializer_list<int64_t>& _zshape)
        : NewBaseAttnParam(_do_fc_qkv_fusion, _batch, _seqlen, _head_num, _head_dim, std::vector<int64_t>(_zshape)){};
    NewBaseAttnParam(bool _do_fc_qkv_fusion, int64_t _batch, int64_t _head_num, int64_t _head_dim,
            VectorParam<TID> _lod, const std::vector<int>& _zshape = {})
        : NewBaseAttnParam(_do_fc_qkv_fusion, _batch, _head_num, _head_dim, _lod,
                  std::vector<int64_t>(_zshape.begin(), _zshape.end())){};
    NewBaseAttnParam(bool _do_fc_qkv_fusion, int64_t _batch, int64_t max_seqlen, int64_t _head_num, int64_t _head_dim, 
            VectorParam<TID> _lod, const std::vector<int>& _zshape = {})
        : NewBaseAttnParam(_do_fc_qkv_fusion, _batch, _head_num, _head_dim, _lod, 
                  std::vector<int64_t>(_zshape.begin(), _zshape.end()), 0.0f, max_seqlen){
    };
    NewBaseAttnParam(bool _do_fc_qkv_fusion, int64_t _batch, int64_t _seqlen, int64_t _head_num, int64_t _head_dim,
            const std::vector<int>& _zshape)
        : NewBaseAttnParam(_do_fc_qkv_fusion, _batch, _seqlen, _head_num, _head_dim,
                  std::vector<int64_t>(_zshape.begin(), _zshape.end())){};
    virtual ~NewBaseAttnParam(){};
    virtual const VectorParam<TID> get_qlod() const = 0;
    virtual const VectorParam<TID> get_kvlod() const = 0;
    virtual const VectorParam<TID> get_vbmap() const = 0;
    virtual Error_t selfcheck(Context* ctx) const = 0;
    virtual void attn_lens_info(
            Context* ctx, int64_t* qlen, int64_t* klen, int64_t* vlen, int64_t* qklen, int64_t* qkvlen) const = 0;
    virtual AttnType_t get_attn_type() const = 0;
    virtual int64_t get_mask_len() const = 0;
    virtual AttnMacMaxPtrType_t get_mac_max_ptr_type() const = 0;
};

enum class QuantType : int { QUANT_INT8, QUANT_INT16, NOT_QUANT };

template <typename TID = int> class DLL_EXPORT NewQKVAttnParam : public NewBaseAttnParam<TID> {
  public:
    int64_t mask_ld;
    const bool is_pre_norm;
    const Activation_t act;
    int64_t last_slice_seq;
    int64_t pad_seqlen;
    int64_t hidden_dim;
    bool is_perchannel;
    int64_t scale_of_hidden_units = 4;
    std::vector<QuantType> quant_type_;
    int64_t qkv_shape = 0;
    AttnMacMaxPtrType_t max_ptr_type;
    int64_t ldz = -1;
    // only for KL1, ignore me if you are not using KL1
    VectorParam<TID> sqlod;
    int64_t relative_type = 0; // 0 no relative, 1: roformer relative
    int64_t max_pos_len = 512;
    std::vector<const float*> relative_pos; // [cos, sin], their shape:[max_pos_len, head_dim]
    // for SmoothQuant
    bool is_smooth_quant = false;
    bool force_pad_v = false;   //TODO: use this param to force padding to v
    std::vector<const float16*> smooth_scale;
    // for PTQ first matmul result
    std::vector<float> ptq_max_value{}; // TODO: move this layer info out of these size
                                        //  pass ptq info from lite to multi_encoder
                                        //  size() = layer number
    float* ptq_pre_softmax_max {nullptr}; // pass ptq from multi_encoder to qkv
    bool triangle_mask_autogen{false}; // generate triangle mask for context attention
    NewQKVAttnParam() = delete;
    NewQKVAttnParam(VectorParam<TID> _lod, int64_t _head_num, int64_t _head_dim,
            const Activation_t& _act = Activation_t::RELU, int64_t _last_slice_seq = -1, bool _do_fc_qkv_fusion = false,
            int64_t _pad_seqlen = -1, int64_t _hidden_dim = -1, bool _is_pre_norm = false, bool _is_perchannel = false,
            int64_t _qkv_shape = 0, const std::vector<int64_t>& _zshape = {},
            AttnMacMaxPtrType_t _max_ptr_type = ATTN_WHOLE_BATCH, int64_t _ldz = -1,
            VectorParam<TID> _sqlod = {nullptr, 0, nullptr}, float _alpha = 0.0f, int _max_seqlen = -1)
        : NewBaseAttnParam<TID>(_do_fc_qkv_fusion, (_lod.len - 1), _head_num, _head_dim, _lod, _zshape, _alpha, _max_seqlen),
          is_pre_norm(_is_pre_norm), act(_act), last_slice_seq(_last_slice_seq), pad_seqlen(_pad_seqlen),
          is_perchannel(_is_perchannel), max_ptr_type(_max_ptr_type), ldz(_ldz), sqlod(_sqlod) {
        hidden_dim = (_hidden_dim == -1 ? this->head_num * this->head_dim : _hidden_dim);
        qkv_shape = _qkv_shape;
        if (this->zshape.size() == 4) {
            ldz = (ldz == -1 ? this->zshape[3] : ldz);
        }
        // Update mask_ld.
        bool valid_mask_shape = (this->zshape.size() == 4);
        valid_mask_shape = valid_mask_shape && (this->zshape[0] == 1 || this->zshape[0] == this->batch);
        valid_mask_shape = valid_mask_shape && (this->zshape[1] == 1 || this->zshape[1] == this->head_num);
        valid_mask_shape = valid_mask_shape && (this->zshape[2] == 1 || this->zshape[2] == this->max_seqlen);
        valid_mask_shape = valid_mask_shape && (this->zshape[3] == this->max_seqlen);
        if (valid_mask_shape) {
            mask_ld = this->zshape[1] * this->zshape[2] * this->zshape[3];
            if (this->zshape[0] == 1) {
                mask_ld = 0;
            }
        }
    }

    // zshape = {a, b, c, d},
    //      a == 1 or batch
    //      b == 1 or head_num
    //      c == 1 or max_seqlen
    //      d == max_seqlen
    NewQKVAttnParam(int64_t _batch, int64_t _max_seqlen, int64_t _head_num, int64_t _head_dim,
            const std::vector<int64_t>& _mask_shape, const Activation_t& _act = Activation_t::RELU,
            int64_t _last_slice_seq = -1, bool _do_fc_qkv_fusion = false, int64_t _hidden_dim = -1,
            bool _is_pre_norm = false, bool _is_perchannel = false, int64_t _qkv_shape = 0,
            AttnMacMaxPtrType_t _max_ptr_type = ATTN_WHOLE_BATCH, int64_t _ldz = -1, float _alpha = 0.0f)
        : NewBaseAttnParam<TID>(_do_fc_qkv_fusion, _batch, _max_seqlen, _head_num, _head_dim, _mask_shape, _alpha),
          is_pre_norm(_is_pre_norm), act(_act), last_slice_seq(_last_slice_seq), pad_seqlen(-1),
          is_perchannel(_is_perchannel), max_ptr_type(_max_ptr_type), ldz(_ldz) {
        this->sqlod.len = 0;
        this->lod.len = 0;
        hidden_dim = (_hidden_dim == -1 ? this->head_num * this->head_dim : _hidden_dim);
        qkv_shape = _qkv_shape;
        bool valid_mask_shape = (this->zshape.size() == 4);
        valid_mask_shape = valid_mask_shape && (this->zshape[0] == 1 || this->zshape[0] == this->batch);
        valid_mask_shape = valid_mask_shape && (this->zshape[1] == 1 || this->zshape[1] == this->head_num);
        valid_mask_shape = valid_mask_shape && (this->zshape[2] == 1 || this->zshape[2] == this->max_seqlen);
        valid_mask_shape = valid_mask_shape && (this->zshape[3] == this->max_seqlen);
        hidden_dim = ((_hidden_dim == -1) ? (this->head_num * this->head_dim) : _hidden_dim);
        if (valid_mask_shape) {
            mask_ld = this->zshape[1] * this->zshape[2] * this->zshape[3];
            if (this->zshape[0] == 1) {
                mask_ld = 0;
            }
        }
        if (this->zshape.size() == 4) {
            ldz = (ldz == -1 ? this->zshape[3] : ldz);
        }
    }
    // RELU
    NewQKVAttnParam(VectorParam<TID> _lod, int64_t _head_num, int64_t _head_dim, const Activation_t& _act,
            int64_t _last_slice_seq, bool _do_fc_qkv_fusion, int64_t _pad_seqlen, int64_t _hidden_dim,
            bool _is_pre_norm, bool _is_perchannel, int64_t _qkv_shape, const std::initializer_list<int64_t>& _zshape,
            AttnMacMaxPtrType_t _max_ptr_type = ATTN_WHOLE_BATCH, int64_t _ldz = -1,
            VectorParam<TID> _sqlod = {nullptr, 0, nullptr}, float _alpha = 0.0f, int _max_seqlen = -1)
        : NewQKVAttnParam(_lod, _head_num, _head_dim, _act, _last_slice_seq, _do_fc_qkv_fusion, _pad_seqlen,
                  _hidden_dim, _is_pre_norm, _is_perchannel, _qkv_shape, std::vector<int64_t>(_zshape), _max_ptr_type,
                  _ldz, _sqlod, _alpha, _max_seqlen) {}
    NewQKVAttnParam(int64_t _batch, int64_t _max_seqlen, int64_t _head_num, int64_t _head_dim,
            const std::initializer_list<int64_t>& _mask_shape, const Activation_t& _act = Activation_t::RELU,
            int64_t _last_slice_seq = -1, bool _do_fc_qkv_fusion = false, int64_t _hidden_dim = -1,
            bool _is_pre_norm = false, bool _is_perchannel = false, int64_t _qkv_shape = 0,
            AttnMacMaxPtrType_t _max_ptr_type = ATTN_WHOLE_BATCH, int64_t _ldz = -1, float _alpha = 0.0f)
        : NewQKVAttnParam(_batch, _max_seqlen, _head_num, _head_dim, std::vector<int64_t>(_mask_shape), _act,
                  _last_slice_seq, _do_fc_qkv_fusion, _hidden_dim, _is_pre_norm, _is_perchannel, _qkv_shape,
                  _max_ptr_type, _ldz, _alpha) {}
    NewQKVAttnParam(VectorParam<TID> _lod, int64_t _head_num, int64_t _head_dim, const Activation_t& _act,
            int64_t _last_slice_seq, bool _do_fc_qkv_fusion, int64_t _pad_seqlen, int64_t _hidden_dim,
            bool _is_pre_norm, bool _is_perchannel, int64_t _qkv_shape, const std::vector<int>& _zshape,
            AttnMacMaxPtrType_t _max_ptr_type = ATTN_WHOLE_BATCH, int64_t _ldz = -1,
            VectorParam<TID> _sqlod = {nullptr, 0, nullptr}, float _alpha = 0.0f, int _max_seqlen = -1)
        : NewQKVAttnParam(_lod, _head_num, _head_dim, _act, _last_slice_seq, _do_fc_qkv_fusion, _pad_seqlen,
                  _hidden_dim, _is_pre_norm, _is_perchannel, _qkv_shape,
                  std::vector<int64_t>(_zshape.begin(), _zshape.end()), _max_ptr_type, _ldz, _sqlod, _alpha, _max_seqlen) {}
    NewQKVAttnParam(int64_t _batch, int64_t _max_seqlen, int64_t _head_num, int64_t _head_dim,
            const std::vector<int>& _mask_shape, const Activation_t& _act = Activation_t::RELU,
            int64_t _last_slice_seq = -1, bool _do_fc_qkv_fusion = false, int64_t _hidden_dim = -1,
            bool _is_pre_norm = false, bool _is_perchannel = false, int64_t _qkv_shape = 0,
            AttnMacMaxPtrType_t _max_ptr_type = ATTN_WHOLE_BATCH, int64_t _ldz = -1, float _alpha = 0.0f)
        : NewQKVAttnParam(_batch, _max_seqlen, _head_num, _head_dim,
                  std::vector<int64_t>(_mask_shape.begin(), _mask_shape.end()), _act, _last_slice_seq,
                  _do_fc_qkv_fusion, _hidden_dim, _is_pre_norm, _is_perchannel, _qkv_shape, _max_ptr_type, _ldz, _alpha) {}
    ~NewQKVAttnParam(){};
    const VectorParam<TID> get_qlod() const { return this->lod; };
    const VectorParam<TID> get_kvlod() const { return this->lod; };
    const VectorParam<TID> get_vbmap() const { return {nullptr, 0, nullptr}; };
    Error_t selfcheck(Context* ctx) const;
    void attn_lens_info(
            Context* ctx, int64_t* qlen, int64_t* klen, int64_t* vlen, int64_t* qklen, int64_t* qkvlen) const;
    AttnType_t get_attn_type() const { return QKV_ATTENTION; };
    int64_t get_mask_len() const {
        return this->zshape.size() == 4 ? this->zshape[0] * this->zshape[1] * this->zshape[2] * ldz : -1;
    };
    AttnMacMaxPtrType_t get_mac_max_ptr_type() const { return max_ptr_type; };
};

// in this attention, lod was used for both q and k/v
// lod[0:batch] : q lod
// lod[batch + 1: 2batch + 1]: klod
template <typename TID = int> class DLL_EXPORT NewDifSeqAttnParam : public NewBaseAttnParam<TID> {
public:
    AttnType_t attn_type = DIF_SEQ_ATTENTION;
    int64_t max_q_seq;
    int64_t max_kv_seq;
    int64_t do_softmax;       // 1 do softmax
    int64_t attn_probs_trans; // 1 tranposed
    bool is_train_grad;
    NewDifSeqAttnParam() = delete;
    NewDifSeqAttnParam(VectorParam<TID> _lod, int64_t _head_num, int64_t _head_dim, int64_t _do_softmax = 1,
            int64_t _attn_probs_trans = 0, const std::vector<int64_t>& _zshape = {}, bool _do_fc_qkv_fusion = 0,
            bool _is_train_grad = 0)
        : NewBaseAttnParam<TID>(_do_fc_qkv_fusion, (_lod.len / 2 - 1), _head_num, _head_dim, _lod, _zshape),
          do_softmax(_do_softmax), attn_probs_trans(_attn_probs_trans), is_train_grad(_is_train_grad) {
        max_q_seq = -1;
        max_kv_seq = -1;
        if (this->lod.cpu[0] == 0 && this->batch >= 1) {
            max_q_seq = this->lod.cpu[1];
            int64_t min_seqlen = this->lod.cpu[1];
            for (int64_t i = 1; i < this->batch; i++) {
                int64_t seqlen = this->lod.cpu[i + 1] - this->lod.cpu[i];
                max_q_seq = std::max<int64_t>(max_q_seq, seqlen);
                min_seqlen = std::min<int64_t>(min_seqlen, seqlen);
            }
            if (min_seqlen <= 0) {
                max_q_seq = -1;
            }
            min_seqlen = this->lod.cpu[this->batch + 2];
            ;
            max_kv_seq = this->lod.cpu[this->batch + 2];
            for (int64_t i = this->batch + 2; i < 2 * this->batch + 1; i++) {
                int64_t seqlen = this->lod.cpu[i + 1] - this->lod.cpu[i];
                max_kv_seq = std::max<int64_t>(max_kv_seq, seqlen);
                min_seqlen = std::min<int64_t>(min_seqlen, seqlen);
            }
            if (min_seqlen <= 0) {
                max_kv_seq = -1;
            }
        }
        this->max_seqlen = std::max(max_q_seq, max_kv_seq);
    }
    NewDifSeqAttnParam(int64_t _batch, int64_t _max_q_seq, int64_t _max_kv_seq, int64_t _head_num, int64_t _head_dim,
            const std::vector<int64_t>& _zshape,
            int64_t _do_softmax = 1, int64_t _attn_probs_trans = 0, bool _do_fc_qkv_fusion = 0, int _is_train_grad = 0):
        NewBaseAttnParam<TID>(_do_fc_qkv_fusion, _batch, std::max(_max_q_seq, _max_kv_seq), _head_num, _head_dim, _zshape),
        max_q_seq(_max_q_seq), max_kv_seq(_max_kv_seq), do_softmax(_do_softmax),
        attn_probs_trans(_attn_probs_trans), is_train_grad(_is_train_grad){
        this->lod.len = 0;
    }
    NewDifSeqAttnParam(VectorParam<TID> _lod, int64_t _head_num, int64_t _head_dim, int64_t _do_softmax, int64_t _attn_probs_trans,
            const std::initializer_list<int64_t>& _zshape, bool _do_fc_qkv_fusion = 0, int _is_train_grad = 0):
        NewDifSeqAttnParam(_lod, _head_num, _head_dim, _do_softmax, _attn_probs_trans, std::vector<int64_t>(_zshape), 
            _do_fc_qkv_fusion, _is_train_grad){
    }
    NewDifSeqAttnParam(int64_t _batch, int64_t _max_q_seq, int64_t _max_kv_seq, int64_t _head_num, int64_t _head_dim,
            const std::initializer_list<int64_t>& _zshape, 
            int64_t _do_softmax = 1, int64_t _attn_probs_trans = 0, bool _do_fc_qkv_fusion = 0, int _is_train_grad = 0):
        NewDifSeqAttnParam(_batch, _max_q_seq, _max_kv_seq, _head_num, _head_dim, std::vector<int64_t>(_zshape), 
            _do_softmax, _attn_probs_trans, _do_fc_qkv_fusion, _is_train_grad) {
    }
    NewDifSeqAttnParam(VectorParam<TID> _lod, int64_t _head_num, int64_t _head_dim, int64_t _do_softmax, int64_t _attn_probs_trans,
            const std::vector<int>& _zshape, bool _do_fc_qkv_fusion = 0, int _is_train_grad = 0):
        NewDifSeqAttnParam(_lod, _head_num, _head_dim, _do_softmax, _attn_probs_trans, std::vector<int64_t>(_zshape.begin(), _zshape.end()), 
            _do_fc_qkv_fusion, _is_train_grad){
    }
    NewDifSeqAttnParam(int64_t _batch, int64_t _max_q_seq, int64_t _max_kv_seq, int64_t _head_num, int64_t _head_dim,
            const std::vector<int>& _zshape,
            int64_t _do_softmax = 1, int64_t _attn_probs_trans = 0, bool _do_fc_qkv_fusion = 0, int _is_train_grad = 0):
        NewDifSeqAttnParam(_batch, _max_q_seq, _max_kv_seq, _head_num, _head_dim, std::vector<int64_t>(_zshape.begin(), _zshape.end()), 
            _do_softmax, _attn_probs_trans, _do_fc_qkv_fusion, _is_train_grad) {
    }

    ~NewDifSeqAttnParam(){};
    const VectorParam<TID> get_qlod() const {
        VectorParam<TID> ret{this->lod.cpu, this->lod.len / 2, this->lod.xpu};
        return ret;
    };
    const VectorParam<TID> get_kvlod() const {
        VectorParam<TID> ret{this->lod.cpu + this->lod.len / 2, this->lod.len / 2, this->lod.xpu + this->lod.len / 2};
        return ret;
    };
    const VectorParam<TID> get_vbmap() const { return {nullptr, 0, nullptr}; };
    Error_t selfcheck(Context* ctx) const;
    void attn_lens_info(
            Context* ctx, int64_t* qlen, int64_t* klen, int64_t* vlen, int64_t* qklen, int64_t* qkvlen) const;
    AttnType_t get_attn_type() const { return DIF_SEQ_ATTENTION; };
    int64_t get_mask_len() const {
        return this->zshape.size() == 4 ? this->zshape[0] * this->zshape[1] * this->zshape[2] * this->zshape[3] : -1;
    };
    AttnMacMaxPtrType_t get_mac_max_ptr_type() const { return ATTN_WHOLE_BATCH; };
};

// In decode, we suppose q is sliced
template <typename TID = int> class DLL_EXPORT NewDecodeAttnParam : public NewBaseAttnParam<TID> {
  public:
    AttnType_t attn_type = DECODE_ATTENTION;
    int64_t step;
    const int64_t qkv_shape;
    int64_t vbatch;
    VectorParam<TID> vb_map;

    // NewDecodeAttnParam(VectorParam<TID> _lod, int _head_num, int _head_dim,  int _step = 1, int _qkv_shape = 0,
    //         int _vbatch = -1, VectorParam<TID> _vb_map = {nullptr, 0, nullptr}):
    //         NewBaseAttnParam(false, (_lod.len - 1), _head_num, _head_dim, _lod, {}),
    //         step(_step), qkv_shape(_qkv_shape), vbatch(_vbatch), vb_map(_vb_map){step = (step == -1 ? 1 : step);};
    NewDecodeAttnParam(VectorParam<TID> _lod, int64_t _max_seqlen, int64_t _head_num, int64_t _head_dim,  int64_t _step = -1, 
            int64_t _qkv_shape = 0, int64_t _vbatch = -1, VectorParam<TID> _vb_map = {nullptr, 0, nullptr})
        : NewBaseAttnParam<TID>(false, (_lod.len) / 2 - 1, _max_seqlen, _head_num, _head_dim, _lod),
          step(_step), qkv_shape(_qkv_shape), vbatch(_vbatch), vb_map(_vb_map){
        vbatch = (vbatch == -1 ? this->batch : vbatch);
    };
    NewDecodeAttnParam() = delete;
    NewDecodeAttnParam(int64_t _batch, int64_t _max_seqlen, int64_t _head_num, int64_t _head_dim, int64_t _step = 1,
            int64_t _qkv_shape = 0, int64_t _vbatch = -1, VectorParam<TID> _vb_map = {nullptr, 0, nullptr},
            const std::vector<int64_t>& _rel_pos_bias_shape = {})
        : NewBaseAttnParam<TID>(false, _batch, _max_seqlen, _head_num, _head_dim, _rel_pos_bias_shape), step(_step),
          qkv_shape(_qkv_shape), vbatch(_vbatch), vb_map(_vb_map) {
        this->lod.len = 0;
        vbatch = (vbatch == -1 ? this->batch : vbatch);
    };

    NewDecodeAttnParam(int64_t _batch, int64_t _max_seqlen, int64_t _head_num, int64_t _head_dim, int64_t _step,
            int64_t _qkv_shape, int64_t _vbatch, VectorParam<TID> _vb_map,
            const std::initializer_list<int64_t>& _rel_pos_bias_shape)
        : NewDecodeAttnParam(_batch, _max_seqlen, _head_num, _head_dim, _step, _qkv_shape, _vbatch, _vb_map,
                  std::vector<int64_t>(_rel_pos_bias_shape)){};
    NewDecodeAttnParam(int64_t _batch, int64_t _max_seqlen, int64_t _head_num, int64_t _head_dim, int64_t _step,
            int64_t _qkv_shape, int64_t _vbatch, VectorParam<TID> _vb_map, const std::vector<int>& _rel_pos_bias_shape)
        : NewDecodeAttnParam(_batch, _max_seqlen, _head_num, _head_dim, _step, _qkv_shape, _vbatch, _vb_map,
                  std::vector<int64_t>(_rel_pos_bias_shape.begin(), _rel_pos_bias_shape.end())){};
    ~NewDecodeAttnParam(){};
    const VectorParam<TID> get_qlod() const { return this->lod; };
    const VectorParam<TID> get_kvlod() const {
        VectorParam<TID> ret{this->lod.cpu + this->lod.len / 2, this->lod.len / 2, this->lod.xpu + this->lod.len / 2};
        return ret;
    };
    const VectorParam<TID> get_vbmap() const { return vb_map; };
    Error_t selfcheck(Context* ctx) const;
    void attn_lens_info(
            Context* ctx, int64_t* qlen, int64_t* klen, int64_t* vlen, int64_t* qklen, int64_t* qkvlen) const;
    AttnType_t get_attn_type() const { return DECODE_ATTENTION; };
    int64_t get_mask_len() const {
        return this->zshape.size() == 4 ? this->zshape[0] * this->zshape[1] * this->zshape[2] * this->zshape[3] : -1;
    };
    AttnMacMaxPtrType_t get_mac_max_ptr_type() const { return ATTN_WHOLE_BATCH; };
};

// this variant is designed to support all usual features of attention
// 1. is_vsl: q/k/v 的seqlen是定长还是变长
// 2. q 和 k/v 的seqlen是否相等，不相等时也就是cross attention。
// 3. is_qkv_fc_fusion。 表明传入attention的Q/K/V三个矩阵是否沿head_num * head_dim 维被concat在一起。一种常见优化
// 4. slice_conf: 描述slice情况。如果slice_conf = 1, 表明q输入时已经被slice过，只保留了一个token，常见于decoder。
//                             如果slice_conf = 0, 表明attention的输出会只保留一个token
//                              default = -1, 表明正常输入正常输出
// 5. zshape: 作用于qk_attention，描述q * k^T 后是否进行掩码或者加rel_pos_bias。 zshape = 4: 表明将接口中的z
// broadcast加到score结果上， = 0， 表示不加，且此时传入的z要是nullptr。
//               历史原因， z 就是 接口中的mask。是否加z不在和定长变长绑定。
// 6. qkv_shape: 描述q/k/v三个矩阵的内存布局。 default = 0, 表明常见的 {batch, seqlen, head_num, head_dim} 布局
//                                          = 1时，表明输入布局为 {seqlen, batch, head_num, head_dim}，
//                                          仅当每个batch的seqlen相等时可以用。
// 7. do_softmax: 作用于qk_attention，描述是否将q * k^T 得到的score矩阵沿最后一维做softmax得到prob矩阵。 true 就是输出
// prob矩阵， false就是输出score矩阵
// 8. attn_probs_trans: 作用于qk_v_atention。 描述qk_v_attention
// 的左矩阵，也就是prob矩阵，是否是经过转置的。为1转置的，layout为 {bs, h, qseqlen, kseqlen}，为0不转置，输入layout为
// {bs, h, kseqlen, qseqlen}
// 9. step: 描述k/v 矩阵的有效seqlen， 功能等价于decoder中step。 defult = -1, 全有效。
// 10. vbath: 有效batch， 描述有效的batch的个数，并使用vbmap 映射哪些batch有效

// 提供定长和变长两个构造函数。
template <typename TID = int> class DLL_EXPORT NewCrossAttnParam : public NewBaseAttnParam<TID> {
  public:
    AttnType_t attn_type = CROSS_ATTENTION;
    int64_t max_q_seq;
    int64_t max_kv_seq;
    int64_t slice_conf;
    int64_t qkv_shape;
    int64_t do_softmax;       // 1 do softmax
    int64_t attn_probs_trans; // 1 tranposed
    int64_t step;
    int64_t vbatch;
    VectorParam<TID> vb_map;
    bool triangle_mask_autogen{false}; // generate triangle mask for context attention

    NewCrossAttnParam() = delete;
    ~NewCrossAttnParam(){};
    // vsl ctr
    NewCrossAttnParam(int64_t _batch, VectorParam<TID> _lod, int64_t _head_num, int64_t _head_dim,
            bool _do_fc_qkv_fusion = false, const std::vector<int64_t>& _zshape = {}, int64_t _slice_conf = -1,
            int64_t _qkv_shape = 0, int64_t _do_softmax = 1, int64_t _attn_probs_trans = 0, int64_t _step = -1,
            int64_t _vbatch = -1, VectorParam<TID> _vb_map = {nullptr, 0, nullptr}, float _alpha = 0.0f)
        : NewBaseAttnParam<TID>(_do_fc_qkv_fusion, _batch, _head_num, _head_dim, _lod, _zshape, _alpha),
          slice_conf(_slice_conf), qkv_shape(_qkv_shape), do_softmax(_do_softmax), attn_probs_trans(_attn_probs_trans),
          step(_step), vbatch(_vbatch), vb_map(_vb_map) {
        max_q_seq = -1;
        max_kv_seq = -1;
        if (this->lod.cpu[0] == 0 && this->batch >= 1) {
            max_q_seq = this->lod.cpu[1];
            int64_t min_seqlen = this->lod.cpu[1];
            int64_t klod_offset = (this->lod.len == (2 * this->batch + 2) ? (this->batch + 1) : 0);
            min_seqlen = this->lod.cpu[klod_offset + 1];
            max_kv_seq = this->lod.cpu[klod_offset + 1];
            for (int64_t i = 1; i < this->batch; i++) {
                int64_t qseqlen = this->lod.cpu[i + 1] - this->lod.cpu[i];
                max_q_seq = std::max<int64_t>(max_q_seq, qseqlen);
                min_seqlen = std::min<int64_t>(min_seqlen, qseqlen);
                int64_t kvseqlen = this->lod.cpu[i + 1 + klod_offset] - this->lod.cpu[i + klod_offset];
                max_kv_seq = std::max<int64_t>(max_kv_seq, kvseqlen);
                min_seqlen = std::min<int64_t>(min_seqlen, kvseqlen);
            }
            if (min_seqlen < 0) {
                max_q_seq = -1;
            }
            if (min_seqlen < 0) {
                max_kv_seq = -1;
            }
        }
        this->max_seqlen = std::max(max_q_seq, max_kv_seq);
        vbatch = (_vbatch == -1 ? _batch : _vbatch);
    };
    // novsl ctr
    NewCrossAttnParam(int64_t _batch, int64_t _max_seqlen, int64_t _head_num, int64_t _head_dim,
            bool _do_fc_qkv_fusion = false, int64_t _max_q_seq = -1, int64_t _max_kv_seq = -1,
            const std::vector<int64_t>& _zshape = {}, int64_t _slice_conf = -1, int64_t _qkv_shape = 0,
            int64_t _do_softmax = 1, int64_t _attn_probs_trans = 0, int64_t _step = -1, int64_t _vbatch = -1,
            VectorParam<TID> _vb_map = {nullptr, 0, nullptr}, float _alpha = 0.0f)
        : NewBaseAttnParam<TID>(
                  _do_fc_qkv_fusion, _batch, std::max(_max_q_seq, _max_kv_seq), _head_num, _head_dim, _zshape, _alpha),
          slice_conf(_slice_conf), qkv_shape(_qkv_shape), do_softmax(_do_softmax), attn_probs_trans(_attn_probs_trans),
          vbatch(_vbatch), vb_map(_vb_map) {
        this->lod.len = 0;
        max_q_seq = (_max_q_seq == -1 ? _max_seqlen : _max_q_seq);
        max_kv_seq = (_max_kv_seq == -1 ? _max_seqlen : _max_kv_seq);
        step = (_step == -1 ? max_kv_seq : _step);
        vbatch = (_vbatch == -1 ? _batch : _vbatch);
        this->max_seqlen = std::max(max_q_seq, max_kv_seq);
    };
    NewCrossAttnParam(int64_t _batch, VectorParam<TID> _lod, int64_t _head_num, int64_t _head_dim,
            bool _do_fc_qkv_fusion, const std::initializer_list<int64_t>& _zshape, int64_t _slice_conf = -1,
            int64_t _qkv_shape = 0, int64_t _do_softmax = 1, int64_t _attn_probs_trans = 0, int64_t _step = -1,
            int64_t _vbatch = -1, VectorParam<TID> _vb_map = {nullptr, 0, nullptr}, float alpha = 0.0f)
        : NewCrossAttnParam(_batch, _lod, _head_num, _head_dim, _do_fc_qkv_fusion, std::vector<int64_t>(_zshape),
                  _slice_conf, _qkv_shape, _do_softmax, _attn_probs_trans, _step, _vbatch, _vb_map, alpha){};
    // novsl ctr
    NewCrossAttnParam(int64_t _batch, int64_t _max_seqlen, int64_t _head_num, int64_t _head_dim, bool _do_fc_qkv_fusion,
            int64_t _max_q_seq, int64_t _max_kv_seq, const std::initializer_list<int64_t>& _zshape,
            int64_t _slice_conf = -1, int64_t _qkv_shape = 0, int64_t _do_softmax = 1, int64_t _attn_probs_trans = 0,
            int64_t _step = -1, int64_t _vbatch = -1, VectorParam<TID> _vb_map = {nullptr, 0, nullptr}, float alpha = 0.0f)
        : NewCrossAttnParam(_batch, _max_seqlen, _head_num, _head_dim, _do_fc_qkv_fusion, _max_q_seq, _max_kv_seq,
                  std::vector<int64_t>(_zshape), _slice_conf, _qkv_shape, _do_softmax, _attn_probs_trans, _step,
                  _vbatch, _vb_map, alpha){};
    NewCrossAttnParam(int64_t _batch, VectorParam<TID> _lod, int64_t _head_num, int64_t _head_dim,
            bool _do_fc_qkv_fusion, const std::vector<int>& _zshape, int64_t _slice_conf = -1, int64_t _qkv_shape = 0,
            int64_t _do_softmax = 1, int64_t _attn_probs_trans = 0, int64_t _step = -1, int64_t _vbatch = -1,
            VectorParam<TID> _vb_map = {nullptr, 0, nullptr}, float alpha = 0.0f)
        : NewCrossAttnParam(_batch, _lod, _head_num, _head_dim, _do_fc_qkv_fusion,
                  std::vector<int64_t>(_zshape.begin(), _zshape.end()), _slice_conf, _qkv_shape, _do_softmax,
                  _attn_probs_trans, _step, _vbatch, _vb_map, alpha){};
    // novsl ctr
    NewCrossAttnParam(int64_t _batch, int64_t _max_seqlen, int64_t _head_num, int64_t _head_dim, bool _do_fc_qkv_fusion,
            int64_t _max_q_seq, int64_t _max_kv_seq, const std::vector<int>& _zshape, int64_t _slice_conf = -1,
            int64_t _qkv_shape = 0, int64_t _do_softmax = 1, int64_t _attn_probs_trans = 0, int64_t _step = -1,
            int64_t _vbatch = -1, VectorParam<TID> _vb_map = {nullptr, 0, nullptr}, float alpha = 0.0f)
        : NewCrossAttnParam(_batch, _max_seqlen, _head_num, _head_dim, _do_fc_qkv_fusion, _max_q_seq, _max_kv_seq,
                  std::vector<int64_t>(_zshape.begin(), _zshape.end()), _slice_conf, _qkv_shape, _do_softmax,
                  _attn_probs_trans, _step, _vbatch, _vb_map, alpha){};
    const VectorParam<TID> get_qlod() const {
        bool is_dif_seq = this->is_vsl ? (this->lod.len == 2 * (this->batch + 1)) : (max_q_seq != max_kv_seq);
        int64_t len = is_dif_seq ? this->lod.len / 2 : this->lod.len;
        VectorParam<TID> ret{this->lod.cpu, len, this->lod.xpu};
        return ret;
    };
    const VectorParam<TID> get_kvlod() const {
        bool is_dif_seq = this->is_vsl ? (this->lod.len == 2 * (this->batch + 1)) : (max_q_seq != max_kv_seq);
        int64_t len = is_dif_seq ? this->lod.len / 2 : this->lod.len;
        int64_t offset = is_dif_seq ? this->lod.len / 2 : 0;
        VectorParam<TID> ret{this->lod.cpu + offset, len, this->lod.xpu + offset};
        return ret;
    };
    const VectorParam<TID> get_vbmap() const { return vb_map; };
    Error_t selfcheck(Context* ctx) const;
    void attn_lens_info(
            Context* ctx, int64_t* qlen, int64_t* klen, int64_t* vlen, int64_t* qklen, int64_t* qkvlen) const;
    AttnType_t get_attn_type() const { return CROSS_ATTENTION; };
    int64_t get_mask_len() const {
        return this->zshape.size() == 4 ? this->zshape[0] * this->zshape[1] * this->zshape[2] * this->zshape[3] : -1;
    };
    // 不在提供这个功能。如果要精度稳定，请自行调用 TGEMM == float 版本
    AttnMacMaxPtrType_t get_mac_max_ptr_type() const { return ATTN_WHOLE_BATCH; };
};
template class NewBaseAttnParam<int>;
template class NewQKVAttnParam<int>;
template class NewDifSeqAttnParam<int>;
template class NewDecodeAttnParam<int>;
template class NewCrossAttnParam<int>;

// for compatibility
typedef NewBaseAttnParam<int> BaseAttnParam;
typedef NewQKVAttnParam<int> QKVAttnParam;
typedef NewDifSeqAttnParam<int> DifSeqAttnParam;
typedef NewDecodeAttnParam<int> DecodeAttnParam;
typedef NewCrossAttnParam<int> CrossAttnParam;

} // namespace api
} // namespace xpu
} // namespace baidu
#endif
