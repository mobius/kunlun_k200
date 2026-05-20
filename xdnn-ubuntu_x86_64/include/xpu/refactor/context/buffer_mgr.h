#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CONTEXT_BUFFER_MGR_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_CONTEXT_BUFFER_MGR_H
#include <xpu/dll_export.h>
#include <stack>
#include <vector>
#include <xpu/runtime.h>

namespace baidu {
namespace xpu {
namespace api {

class ContextImpl;

class DLL_EXPORT BufferMgr {
public:
    BufferMgr(bool do_malloc_when_run_out = true);
    ~BufferMgr();
    static const int alignment = 64;
    int set(void* ptr, size_t size, bool do_free_at_destruction = false);
    void* get_ptr() {
        return _ptr;
    }
    size_t get_size() {
        return _size;
    }
    size_t get_max_mem_demand() {
        return _max_mem_demand;
    }
    void dump_mem_demand(bool do_mem_demand_dump) {
        _do_mem_demand_dump = do_mem_demand_dump;
    }
    
private:
    size_t _cur_mem_demand = 0;
    size_t _max_mem_demand = 0;
    std::stack<std::vector<size_t>> _mem_demand_vec_to_free;
    bool _do_mem_demand_dump = false;
    void* _ptr;
    size_t _size;
    bool _do_malloc_when_run_out;
    bool _do_free_at_destruction;
    std::stack<void*> _ptr_vec;
    std::stack<size_t> _size_vec;
    std::stack<std::vector<void*>> _ptr_vec_to_free;
    std::vector<void*> _free_pending_list;
    void* alloc(ContextImpl* ctxImpl, size_t size, void* xpu_stream);
    void save();
    void restore(void* xpu_stream);
    friend class ctx_guard;
};

}
}
}
#endif
