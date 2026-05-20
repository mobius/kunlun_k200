#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XPU1_ACT_TABLE_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_XPU1_ACT_TABLE_H
#include <xpu/runtime.h>
#include "xpu/refactor/context/xpu_act_type.h"

namespace baidu {
namespace xpu {
namespace api {

// packed_id {
//      param       :       19bit   :   (packed_id >> 13)
//      table_type  :       4bit    :   (packed_id >> 9) & 0xf
//      table_mode  :       3bit    :   (packed_id >> 6) & 0x7
//      table_acc   :       2bit    :   (packed_id >> 4) & 0x3
//      act_type    :       4bit    :   packed_id & 0xf
// }
class DLL_EXPORT XPU1ActTable {
public:
    XPU1ActTable() {};
    ~XPU1ActTable() {
        for (auto& it : _tables) {
            if (it.second != nullptr) {
                xpu_free(it.second);
            }
        }
    };
    // std::tuple<float*, uint32_t>: <pointer, packed_id>
    int get_relu_table(std::tuple<float*, uint32_t>* ret);
    int get_sigmoid_table(std::tuple<float*, uint32_t>* ret);
    int get_tanh_table(std::tuple<float*, uint32_t>* ret);
    int get_gelu_table(std::tuple<float*, uint32_t>* ret);
    int get_approximate_gelu_table(std::tuple<float*, uint32_t>* ret);
    int get_swish_table(std::tuple<float*, uint32_t>* ret);
    int get_leaky_relu_table(std::tuple<float*, uint32_t>* ret, float alpha);
    int get_leaky_relu_bwd_table(std::tuple<float*, uint32_t>* ret, float alpha);
    int get_hard_swish_table(std::tuple<float*, uint32_t>* ret);
    int get_hard_sigmoid_table(std::tuple<float*, uint32_t>* ret, float slope);
    int get_relu6_table(std::tuple<float*, uint32_t>* ret);
    int get_relu_bwd_table(std::tuple<float*, uint32_t>* ret);
    int get_gelu_bwd_table(std::tuple<float*, uint32_t>* ret);
    // exp is not directly used
    int get_exp_table(std::tuple<float*, uint32_t>* ret);
    int get_act_table(const Activation_t& act, std::tuple<float*, uint32_t>* ret) {
        switch (act.type) {
        case Activation_t::LINEAR:
            *ret = std::make_tuple(nullptr, 0);
            return 0;
        case Activation_t::RELU:
            return get_relu_table(ret);
        case Activation_t::SIGMOID:
            return get_sigmoid_table(ret);
        case Activation_t::TANH:
            return get_tanh_table(ret);
        case Activation_t::GELU:
            return get_gelu_table(ret);
        case Activation_t::SWISH:
            return get_swish_table(ret);
        case Activation_t::LEAKY_RELU:
            return get_leaky_relu_table(ret, act.leaky_alpha);
        case Activation_t::HARD_SWISH:
            return get_hard_swish_table(ret);
        case Activation_t::HARD_SIGMOID:
            return get_hard_sigmoid_table(ret, act.hard_sigmoid_slope);
        case Activation_t::RELU6:
            return get_relu6_table(ret);
        case Activation_t::APPROXIMATE_GELU:
            return get_approximate_gelu_table(ret);
        default:
            return 1;
        }
    }
private:
    std::map<int, float*> _tables; // table_id -> pointer
};

}
}
}
#endif
