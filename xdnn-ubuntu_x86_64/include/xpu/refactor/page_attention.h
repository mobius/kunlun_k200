#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CONTEXT__PAGE_ATTENTION_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CONTEXT__PAGE_ATTENTION_H

#include "xpu/dll_export.h"
#include "xpu/refactor/context/newcontext.h"
#include "xpu/xdnn_types.h"
#include <vector>
#ifdef _MSC_VER
#include <algorithm> // std::max, std::min
#endif

namespace baidu {
namespace xpu {
namespace api {

template <typename TID = int> class DLL_EXPORT PageAttnParam {
public:
    int64_t block_size;             // block 在seqlen 维度的大小
    int64_t max_batch_size;         // block table 的第一个维度
    int64_t max_num_blocks_per_seq; // block table 的第②个维度
    VectorParam<TID> real_batch;    // 映射 block table 里真正的 batch
    int quant_type;                 // 0 表示 per cache 量化，1 表示per head 量化
    int page_layout;                // 目前只能选择 『HLD』其中L表示block size 维度

public:
    PageAttnParam() = delete;
    // those were made for supporting int64_t
    PageAttnParam(int64_t block_size_,
                  int64_t max_batch_size_,
                  int64_t max_num_blocks_per_seq_,
                  VectorParam<TID> real_batch_,
                  int quant_type_ = 0,
                  const std::string& page_layout_ = "HLD")
        : block_size(block_size_), max_batch_size(max_batch_size_), max_num_blocks_per_seq(max_num_blocks_per_seq_),
          real_batch(real_batch_), quant_type(quant_type_), page_layout(0){};
};

template class PageAttnParam<int>;

} // namespace api
} // namespace xpu
} // namespace baidu
#endif
