#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CORE_TENSOR_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CORE_TENSOR_H
#include <vector>
#include <string>
#include <cstring> // memset
#include <cstdlib> // malloc
#include <random> // random
#include <stdexcept>    // std::invalid_argument()
#include <new>          // std::bad_alloc()
#ifdef _MSC_VER
#else
#include <sys/time.h>
#endif
#include <sstream>
#include <time.h>
#include <gflags/gflags.h>
#include <xpu/runtime.h>
#include "xpu/refactor/core/device.h"
#include "xpu/refactor/core/dtype.h"

#define TENSOR_THROW(msg)                \
        throw std::invalid_argument(msg);

#define TENSOR_ALLOC_ASSERT_WITHRET(ret) \
        if (ret != 0) {                  \
            do_free();                   \
            TENSOR_THROW("Tensor copy fail"); \
        }

#define TENSOR_ALLOC_ASSERT_WITHPTR(ptr, sz, kind) \
        if (ptr == nullptr) {            \
            do_free();                   \
            std::ostringstream oss;      \
            oss << "Tensor malloc fail, kind = " << kind \
                << ", size = " << sz;    \
            TENSOR_THROW(oss.str());     \
        }

DECLARE_int32(seed); // FLAGS_seed
DECLARE_bool(print_tensor_seed);
DECLARE_bool(print_tensor_diff);
DECLARE_bool(print_tensor);
DECLARE_bool(print_all_diff);
DECLARE_int32(unittest_phase);
DECLARE_string(npy_file_path);
DECLARE_bool(perf_only);

//tensor init varible
static int gtest_base_seed = -1;
//gtest init function
void init_seed(const char* str_used_by_hash);
int xpu_seed();
bool get_gtest_print_all_diff();
bool get_gtest_print_tensor_seed();
bool get_gtest_print_tensor();
bool get_gtest_print_tensor_diff();
bool get_gtest_perf_only();

namespace baidu {
namespace xpu {
namespace api {
// http://agroup.baidu.com/xdnn/md/article/4301359

enum class InOrOutKind {
    INPUT = 0,          // input tensor
    OUTPUT = 1,         // output tensor
    INNOUT = 2,         // input & output tensor
    QINPUT = 3,         // input tensor which needs to be quanted
    QOUTPUT = 4,        // output tensor which needs to be quanted
    QINNOUT = 5,        // input tensor which needs to be quanted
};

constexpr InOrOutKind kINPUT = InOrOutKind::INPUT;
constexpr InOrOutKind kOUTPUT = InOrOutKind::OUTPUT;
constexpr InOrOutKind kINNOUT = InOrOutKind::INNOUT;
constexpr InOrOutKind kQINPUT = InOrOutKind::QINPUT;
constexpr InOrOutKind kQOUTPUT = InOrOutKind::QOUTPUT;
constexpr InOrOutKind kQINNOUT = InOrOutKind::QINNOUT;

std::string to_string(const InOrOutKind& k);

InOrOutKind get_val_tensor_in_or_out_kind(InOrOutKind kind);

InOrOutKind get_max_tensor_in_or_out_kind(InOrOutKind kind);

bool is_same_in_or_out_kind(InOrOutKind kind1, InOrOutKind kind2);
class Tensor {
public:
    inline Tensor():
        _dev(kCPU),
        _dtype(kFLOAT32),
        _ptr(nullptr),
        _numel(1),
        _mem_kind(XPU_MEM_MAIN),
        _in_or_out_kind(InOrOutKind::INPUT),
        _tensor_name("unknown") {
        _shape.push_back(1);
        _ref = this;
        do_malloc();
    }
    // copy constructor
    inline Tensor(const Tensor& t);
    // constructor using len. 1-D tensor created.
    inline Tensor(int64_t len, Dtype in_dtype, InOrOutKind kind = InOrOutKind::INPUT,
            const std::string& tname = "unknown");
    // constructor using shape.
    inline Tensor(const std::vector<int64_t>& shape, Dtype in_type, InOrOutKind kind = InOrOutKind::INPUT,
            const std::string& tname = "unknown");
    inline Tensor(Tensor&& src);
    inline Tensor& operator=(const Tensor& t);
    inline Tensor& operator=(Tensor&& t);
    inline ~Tensor() {
        do_free();
    }
    // get attributes
    Device dev() const {
        return _dev;
    }
    Dtype dtype() const {
        return _dtype;
    }
    template <typename T> T* data() {
        return reinterpret_cast<T*>(_ref->_ptr);
    }
    const std::vector<int64_t>& shape() const {
        return _shape;
    }
    inline int64_t numel() const {
        return _numel;
    }
    inline XPUMemoryKind mem_kind() const {
        return _mem_kind;
    }
    inline InOrOutKind in_or_out_kind() const {
        return _in_or_out_kind;
    }
    inline std::string tensor_name() const {
        return _tensor_name;
    }
    inline std::string npy_file_name() const {
        return _tensor_name + "_" + api::to_string(this->_dev.type()) + ".npy";
    }
    inline void set_in_or_out_kind(InOrOutKind kind) {
        this->_in_or_out_kind = kind;
    }
    inline void set_tensor_name(const std::string& tname) {
        this->_tensor_name = tname;
    }
    inline void force_init(bool force_init) {
        this->_force_init = force_init;
    }
    void reset_ptr() {
        _ptr = nullptr;
        _ref = this;
    }
    DLL_EXPORT void share(Tensor* other); // for tensor reuse
    DLL_EXPORT int save_to_npy(InOrOutKind kind);
    DLL_EXPORT int load_from_npy(InOrOutKind kind);
    DLL_EXPORT int load_from_npy(api::DeviceType dev, InOrOutKind kind);
    // get a modified tensor
    DLL_EXPORT Tensor reshape(const std::vector<int64_t>& in_shape) const;
    // TODO: need to remove initializer_list type when int64_t is ready for all ops
    DLL_EXPORT Tensor reshape(const std::initializer_list<int64_t>& in_shape) const;
    // TODO: need to remove int type when int64_t is ready for all ops
    DLL_EXPORT Tensor reshape(const std::vector<int>& in_shape) const;
    DLL_EXPORT Tensor astype(Dtype in_dtype) const;
    DLL_EXPORT Tensor to(Device in_dev) const;
    DLL_EXPORT void to_l3(Device in_dev, void* allocated_l3_ptr = nullptr);
    DLL_EXPORT inline void do_memcpy(const Tensor& t);             // cpy t's data to this, fail to exit

private:
    inline void do_malloc();            // malloc _ptr, fail to exit
    inline void relocate_mem(Device dev, XPUMemoryKind mem_kind, void* allocated_ptr = nullptr);
    inline void do_free() noexcept;                     // free _ptr and _ptr = nullptr;
    bool is_nullptr() const {
        return _ref->_ptr == nullptr;   // users will never use this function, because in their view, all tensors' ptr is not nullptr.
    }
    // nullptr tensor will cause exception and exit before passing to users
    Device _dev;                        // device type: CPU, XPU1, XPU2
    Dtype _dtype;                       // data type
    void* _ptr;                         // pointer to data.
    // bool _is_shared;                     // whether reused by other tensor
    Tensor* _ref;                        // point to share tensor
    bool _force_init = false;                   // whether force init
    std::vector<int64_t> _shape;        // tensor shape.
    int64_t _numel;                     // tensor numel = shape[0] * shape[1] *...*shape[shape.size()-1]
    XPUMemoryKind _mem_kind;            // memory kind: MAIN_MEM/L3, if dev == cpu, then _mem_kind = XPU_MAIN_MEM
    InOrOutKind _in_or_out_kind;        // input / output / in&out tensor
    std::string _tensor_name;         // tensor_name
};

void Tensor::do_malloc() {
    int64_t mem_sz = static_cast<int64_t>(numel()) * Dtype_size(dtype());
    int64_t max_memory_size = static_cast<int64_t>(4 * 1024 * 1024) * 1024;
    if (mem_sz <= 0l || ((dev().type() == kXPU1 || dev().type() == kXPU2) && mem_sz > max_memory_size)) {
        TENSOR_THROW("Tensor memory size should be smaller than 4GB");
    }
    do_free();                          // to avoid memory leakage
    if (dev().type() == kCPU) {
        _ptr = malloc(mem_sz);
    }
    if (dev().type() != kCPU) {
        xpu_malloc((void**)(&_ptr), mem_sz, _mem_kind);
    }
    TENSOR_ALLOC_ASSERT_WITHPTR(_ptr, mem_sz, _mem_kind);
    return ;
}

void Tensor::do_memcpy(const Tensor& t) {
    if (get_gtest_perf_only() && !t._force_init) {
        printf("In perf only mode, skip copy.\n");
        return;
    }
    if ((is_nullptr()) || (t.is_nullptr())) {
        TENSOR_THROW("Invalid tensor when do_memcpy...");
    }
    if (numel() != t.numel()) {
        TENSOR_THROW("Differnet numel() when do_memcpy...");
    }
    if (dtype() != t.dtype()) {
        TENSOR_THROW("Different data type when do_memcpy...");
    }
    int64_t mem_sz = numel() * Dtype_size(dtype());
    int ret = -1;
    int ret1 = -1;
    if ((dev().type() == kCPU) && (t.dev().type() == kCPU)) {
        std::memcpy(_ptr, t._ref->_ptr, mem_sz);                          // std::memcpy do exception management itself
        ret = 0;
    }
    if ((dev().type() == kCPU) && (t.dev().type() != kCPU)) {
        ret = xpu_memcpy((void*)_ref->_ptr, (const void*)t._ref->_ptr, mem_sz, XPUMemcpyKind::XPU_DEVICE_TO_HOST);
    }
    if ((dev().type() != kCPU) && (t.dev().type() == kCPU)) {
        ret = xpu_memcpy((void*)_ref->_ptr, (const void*)t._ref->_ptr, mem_sz, XPUMemcpyKind::XPU_HOST_TO_DEVICE);
    }
    if ((dev().type() != kCPU) && (t.dev().type() != kCPU)) {
        // ret = xpu_memcpy((void*)_ptr, (const void*)t._ptr, mem_sz, XPUMemcpyKind::XPU_DEVICE_TO_DEVICE);
        // not supported by XPU2 now
        std::vector<char> middle(mem_sz);
        ret = xpu_memcpy((void*)(middle.data()), (const void*)t._ref->_ptr, mem_sz, XPUMemcpyKind::XPU_DEVICE_TO_HOST);
        ret1 = xpu_memcpy((void*)_ref->_ptr, (void*)(middle.data()), mem_sz, XPUMemcpyKind::XPU_HOST_TO_DEVICE);
        TENSOR_ALLOC_ASSERT_WITHRET(ret1);
    }
    TENSOR_ALLOC_ASSERT_WITHRET(ret);
    return;
}

void Tensor::relocate_mem(Device dev, XPUMemoryKind mem_kind = XPU_MEM_MAIN, void* allocated_ptr) {
    // 1. same device and same mem_kind
    // if _dev == cpu, _mem_kind will always be same
    if (_dev == dev && _mem_kind == mem_kind) {
        return;                                 // do nothing
    }
    // others situations need do memory copy
    int64_t mem_sz = numel() * Dtype_size(dtype());
    int ret = -1;
    // middle data
    std::vector<char> middle(mem_sz);
    if (_dev == kCPU) {
        std::memcpy((void*)(middle.data()), (const void*)_ref->_ptr, mem_sz);
    } else {
        if (!get_gtest_perf_only() || _ref->_force_init) {
            ret = xpu_memcpy((void*)(middle.data()), (const void*)_ref->_ptr, mem_sz, XPUMemcpyKind::XPU_DEVICE_TO_HOST);
            // check middle data copy success
            TENSOR_ALLOC_ASSERT_WITHRET(ret);
        }
    }

    // other situations
    // first do free, then reset device status, finnally copy middle data to dst
    do_free();
    _dev = dev;
    _mem_kind = mem_kind;
    if (allocated_ptr == nullptr) {
        do_malloc();
    } else {
        _ref->_ptr = allocated_ptr;
    }
    if (dev == kCPU) {
        std::memcpy((void*)_ref->_ptr, (void*)(middle.data()), mem_sz);
    } else {
        if (!get_gtest_perf_only() || _ref->_force_init) {
            ret = xpu_memcpy((void*)_ref->_ptr, (void*)(middle.data()), mem_sz, XPUMemcpyKind::XPU_HOST_TO_DEVICE);
            // check relocation success
            TENSOR_ALLOC_ASSERT_WITHRET(ret);
        }
    }
    return;
}
void Tensor::do_free() noexcept {
    if (_ref == this && mem_kind() != XPU_MEM_L3) { // to avoid double free; L3 use RAII alloc
        if (_ptr != nullptr) {
            if (dev().type() == kCPU) {
                free(_ptr);
            }
            if (dev().type() != kCPU) {
                xpu_free(_ptr);
                xpu_wait();
            }
        }
    }
    _ptr = nullptr;                         // to avoid double free
    _ref = this;
    return;
}

Tensor::Tensor(int64_t len, Dtype in_dtype, InOrOutKind kind, const std::string& tname) {
    _ref = this;
    _ptr = nullptr;
    if (len <= 0) {
        TENSOR_THROW("Tensor numel should be positive.");
    }
    _dtype = in_dtype;
    _dev = {kCPU, 0};
    _shape.push_back(len);
    _numel = len;
    _mem_kind = XPU_MEM_MAIN;
    _in_or_out_kind = kind;
    _tensor_name = tname;
    if (_ref == this) {
        do_malloc();
    }
}

Tensor::Tensor(const std::vector<int64_t>& shape, Dtype in_type, InOrOutKind kind, const std::string& tname) {
    _ref = this;
    _ptr = nullptr;
    _dtype = in_type;
    _numel = 1;
    for (size_t i = 0; i < shape.size(); i++) {
        _numel = _numel * shape[i];
        if (shape[i] < 0) {
            TENSOR_THROW("Invalid shape size");
        }
    }
    if (_numel <= 0) {
        TENSOR_THROW("Tensor numel should be positive.");
    }
    _dev = {kCPU, 0};
    _shape = shape;
    _mem_kind = XPU_MEM_MAIN;
    _in_or_out_kind = kind;
    _tensor_name = tname;
    do_malloc();
}
Tensor::Tensor(const Tensor& t) {
    _ref = t._ref == &t ? this : t._ref; // check src whether is shared
    _ptr = nullptr;
    // do not construct a tensor with another tensor with nulltpr.
    if (t.is_nullptr()) {
        TENSOR_THROW("Invalid Tensor");        // early exit, to avoid useless do_malloc() when t's ptr is nullptr
    }
    _dev = t.dev();
    _dtype = t.dtype();
    _shape = t.shape();
    _numel = t.numel();
    _mem_kind = t.mem_kind();       // temporary
    _in_or_out_kind = t.in_or_out_kind();
    _tensor_name = t.tensor_name();
    _force_init = t._force_init;
    if (_ref == this) {
        do_malloc();      // if failed, throw exception and directly exit
        do_memcpy(t);
    }
}

Tensor& Tensor::operator=(const Tensor& t) {
    if (this == &t) {
        return *this;
    }
    do_free();
    // do not assign a tensor with nullptr to another one.
    if (t.is_nullptr()) {
        TENSOR_THROW("Invalid Tensor");
    }
    _ref = t._ref == &t ? this : t._ref; // check src whether is shared
    _dev = t._dev;
    _dtype = t._dtype;              // will do data type transformation defaultly.
    _shape = t._shape;
    _numel = t._numel;              // for support tensor reuse.
    _force_init = t._force_init;
    _mem_kind = t.mem_kind();       // temporary
    _in_or_out_kind = t.in_or_out_kind();
    _tensor_name = t.tensor_name();
    if (_ref == this) {
        do_malloc();                    // if failed, throw exception and directly exit
        do_memcpy(t);                   // if do_memcpy() success, then numel is same naturaly.
    }
    return *this;
}

Tensor& Tensor::operator=(Tensor&& t) {
    if (this == &t) {
        return *this;
    }
    do_free();
    // do not assign a tensor with nullptr to another one.
    if (t.is_nullptr()) {
        TENSOR_THROW("Invalid Tensor");
    }
    _ref = t._ref == &t ? this : t._ref; // check src whether is shared
    _dev = t._dev;
    _dtype = t._dtype;              // will do data type transformation defaultly.
    _shape = t._shape;
    _numel = t._numel;              // for support tensor reuse.
    _force_init = t._force_init;
    _mem_kind = t.mem_kind();       // temporary
    _in_or_out_kind = t.in_or_out_kind();
    _tensor_name = t.tensor_name();
    _ptr = t._ptr;
    t.reset_ptr();
    return *this;
}

Tensor::Tensor(Tensor&& src) {
    _ref = src._ref == &src ? this : src._ref; // check src whether is shared
    _dev = src.dev();
    _dtype = src.dtype();
    _shape = src.shape();
    _numel = src.numel();
    _mem_kind = src.mem_kind();
    _in_or_out_kind = src.in_or_out_kind();
    _tensor_name = src.tensor_name();
    _ptr = src._ptr;
    src.reset_ptr();
}

// Creation
DLL_EXPORT Tensor tensor(int val, InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown");
DLL_EXPORT Tensor tensor(float val, InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown");
DLL_EXPORT Tensor tensor(const std::vector<int8_t>& vec, InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown");
DLL_EXPORT Tensor tensor(const std::vector<int16_t>& vec, InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown");
DLL_EXPORT Tensor tensor(const std::vector<int>& vec, InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown");
DLL_EXPORT Tensor tensor(const std::vector<int64_t>& vec, InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown");
DLL_EXPORT Tensor tensor(const std::vector<float16>& vec, InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown");
DLL_EXPORT Tensor tensor(const std::vector<float>& vec, InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown");
DLL_EXPORT Tensor tensor(const std::string& fname, Dtype dtype, InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown");
DLL_EXPORT Tensor tensor(const std::string& fname, InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown");
DLL_EXPORT Tensor arange(int start, int end, int step, InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown");
DLL_EXPORT Tensor arange(float start, float end, float step, InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown");
DLL_EXPORT Tensor randint(int minval, int maxval, int64_t size, int seed = xpu_seed(), InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown", bool force_init = false);
DLL_EXPORT Tensor norepeat_randint(int minval, int maxval, int64_t size, int seed = xpu_seed(), InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown", bool force_init = false);
DLL_EXPORT Tensor randfloat(float minval, float maxval, int64_t size, int seed = xpu_seed(), InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown", bool force_init = false);
DLL_EXPORT Tensor norepeat_randfloat(float minval, float maxval, int64_t size, int seed = xpu_seed(), InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown", bool force_init = false);
DLL_EXPORT Tensor randint(int minval, int maxval, const std::vector<int64_t>& shape, int seed = xpu_seed(), InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown", bool force_init = false);
DLL_EXPORT Tensor norepeat_randint(int minval, int maxval, const std::vector<int64_t>& shape, int seed = xpu_seed(), InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown", bool force_init = false);
DLL_EXPORT Tensor randfloat(float minval, float maxval, const std::vector<int64_t>& shape, int seed = xpu_seed(), InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown", bool force_init = false);
DLL_EXPORT Tensor sorted_randfloat(float minval, float maxval, int64_t size, int seed = xpu_seed(), InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown", bool force_init = false);
DLL_EXPORT Tensor norepeat_randfloat(float minval, float maxval, const std::vector<int64_t>& shape, int seed = xpu_seed(), InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown", bool force_init = false);
// TODO: need to remove initializer_list type shape when int64_t is ready for all ops
DLL_EXPORT Tensor randint(int minval, int maxval, const std::initializer_list<int64_t>& shape, int seed = xpu_seed(), InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown", bool force_init = false);
// TODO: need to remove initializer_list type shape when int64_t is ready for all ops
DLL_EXPORT Tensor norepeat_randint(int minval, int maxval, const std::initializer_list<int64_t>& shape, int seed = xpu_seed(), InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown", bool force_init = false);
// TODO: need to remove initializer_list type shape when int64_t is ready for all ops
DLL_EXPORT Tensor randfloat(float minval, float maxval, const std::initializer_list<int64_t>& shape, int seed = xpu_seed(), InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown", bool force_init = false);
// TODO: need to remove initializer_list type shape when int64_t is ready for all ops
DLL_EXPORT Tensor norepeat_randfloat(float minval, float maxval, const std::initializer_list<int64_t>& shape, int seed = xpu_seed(), InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown", bool force_init = false);
// TODO: need to remove int type shape when int64_t is ready for all ops
DLL_EXPORT Tensor randint(int minval, int maxval, const std::vector<int>& shape, int seed = xpu_seed(), InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown", bool force_init = false);
// TODO: need to remove int type shape when int64_t is ready for all ops
DLL_EXPORT Tensor norepeat_randint(int minval, int maxval, const std::vector<int>& shape, int seed = xpu_seed(), InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown", bool force_init = false);
// TODO: need to remove int type shape when int64_t is ready for all ops
DLL_EXPORT Tensor randfloat(float minval, float maxval, const std::vector<int>& shape, int seed = xpu_seed(), InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown", bool force_init = false);
// TODO: need to remove int type shape when int64_t is ready for all ops
DLL_EXPORT Tensor norepeat_randfloat(float minval, float maxval, const std::vector<int>& shape, int seed = xpu_seed(), InOrOutKind kind = InOrOutKind::INPUT, const std::string& tname = "unknown", bool force_init = false);

DLL_EXPORT std::string to_string(const std::vector<int64_t>& vec);
DLL_EXPORT int64_t count_allclose(const Tensor& t0, const Tensor& t1, float rtol, float atol, int64_t begin = -1, int64_t end = -1);
template<typename TOUT, typename TGEMM> DLL_EXPORT bool check_snr(const Tensor& t0, const Tensor& t1, int64_t acc_dim, bool has_act);
DLL_EXPORT bool abs_max_close(const Tensor& t0, const Tensor& t1, float rtol, float atol);
DLL_EXPORT int print_tensor_diff(const Tensor& t0, const Tensor& t1, int64_t begin = -1, int64_t end = -1);
DLL_EXPORT int print_tensor(const Tensor& t0);
// DLL_EXPORT int debug_level();

}
}
}
#endif
