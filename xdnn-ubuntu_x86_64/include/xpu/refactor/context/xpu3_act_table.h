#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XPU3_ACT_TABLE_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XPU3_ACT_TABLE_H
#include <xpu/runtime.h>
#include "xpu/refactor/context/xpu_act_type.h"
#include <vector>
#include <tuple>
namespace baidu {
namespace xpu {
namespace api {

// packed_id {
//       param               :       16bit   :   (packed_id >> 16)
//       inter_mode          :       1bit    :   (packed_Id >> 15) & 0x1
//       data_type           :       1bit    :   (packed_id >> 14) & 0x1
//       inter_vld|split_vld :       2bit    :   (packed_id >> 12) & 0x3
//       split_full_mode     :       2bit    :   (packed_id >> 10) & 0x3
//       lut_mode            :       2bit    :   (packed_id >> 8) & 0x3
//       lut_range           :       4bit    :   (packed_id >> 4) & 0xf
//       act_type            :       4bit    :   packed_id & 0xf
// }
//
// layout of table
// | k table (#slot_cnt) | b table (#slot_cnt) | lut_min (#1) |

class DLL_EXPORT XPU3ActTable {
public:
    XPU3ActTable() {};
    ~XPU3ActTable() {
        for (auto& it : _tables) {
            if (it.second != nullptr) {
                xpu_free(it.second);
            }
        }
    };
    // one kernel can only use one single table
    int get_relu_table(std::tuple<float*, uint32_t>* ret);
    int get_sigmoid_table(std::tuple<float*, uint32_t>* ret);
    int get_tanh_table(std::tuple<float*, uint32_t>* ret);
    int get_gelu_table(std::tuple<float*, uint32_t>* ret);
    int get_approximate_gelu_table(std::tuple<float*, uint32_t>* ret);
    int get_approximate_gelu_bwd_table(std::tuple<float*, uint32_t>* ret);
    int get_quick_gelu_table(std::tuple<float*, uint32_t>* ret);
    int get_swish_table(std::tuple<float*, uint32_t>* ret);
    int get_relu6_table(std::tuple<float*, uint32_t>* ret);
    int get_hard_swish_table(std::tuple<float*, uint32_t>* ret);
    int get_hard_sigmoid_table(std::tuple<float*, uint32_t>* ret, float slope);
    int get_leaky_relu_table(std::tuple<float*, uint32_t>* ret, float alpha);
    int get_elu_table(std::tuple<float*, uint32_t>* ret, float alpha);
    int get_mish_table(std::tuple<float*, uint32_t>* ret);
    int get_gelu_bwd_table(std::tuple<float*, uint32_t>* ret);
    int get_erf_table(std::tuple<float*, uint32_t>* ret);
    int get_exp_table(std::tuple<float*, uint32_t>* ret);    // exp is not directly used
    int get_exp_inter_table(std::tuple<float*, uint32_t>* ret);    // exp full nterpolation table
    int get_limit_exp_table(std::tuple<float*, uint32_t>* ret, int sqrt_cnt); // special exp (exp with several sqrt)
    int get_direct_exp_table(std::tuple<float*, uint32_t>* ret);    // exp is not directly used
    int get_scaled_exp_table(std::tuple<float*, uint32_t>* ret, int scale = 0);
    int get_swish_glu_gpt_fc_table(std::tuple<float*, uint32_t>* ret);
    int get_ptq_exp_table(std::tuple<float*, uint32_t>* ret, float ptq_value); // special exp (exp with ptq)
    int get_softmax_1_exp_table(std::tuple<float*, uint32_t>* ret);    // exp including positive X
    // general api
    int get_act_table(const Activation_t& act, std::tuple<float*, uint32_t>* ret);
private:
    std::map<std::tuple<int, float>, float*> _tables; // table_id -> pointer
};

}
}
}
#endif
