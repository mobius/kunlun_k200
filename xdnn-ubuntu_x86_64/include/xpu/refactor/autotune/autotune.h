#ifndef __XPU_KERNEL_AUTOTUNE_H
#define __XPU_KERNEL_AUTOTUNE_H
#include "xpu/xdnn.h"
#include "xpu/refactor/impl_public/wrapper_check.h"
#include <float.h>
#include <assert.h>
#include <fstream>
#include <iostream>

namespace baidu {
namespace xpu {
namespace api {

class AutotuneTimer {
public:
    std::chrono::time_point<std::chrono::steady_clock> _cpu_start;
    std::chrono::time_point<std::chrono::steady_clock> _cpu_end;
    uint64_t _xpu_start;
    uint64_t _xpu_end;

    AutotuneTimer() : _xpu_start(0), _xpu_end(0) {}
    int start(baidu::xpu::api::Context* ctx) {
        int ret = xpu_last_kernel_exec_time(&_xpu_start);
        WRAPPER_ASSERT_SUCCESS(ctx, ret);
        _cpu_start = std::chrono::steady_clock::now();
        return baidu::xpu::api::SUCCESS;
    }
    int stop(baidu::xpu::api::Context* ctx) {
        _cpu_end = std::chrono::steady_clock::now();
        int ret = xpu_last_kernel_exec_time(&_xpu_end);
        WRAPPER_ASSERT_SUCCESS(ctx, ret);
        return baidu::xpu::api::SUCCESS;
    }
    float duration(const int loop_time) {
        long long diff = std::chrono::duration_cast<std::chrono::nanoseconds>(_cpu_end - _cpu_start).count();
        if (_xpu_end > _xpu_start) {
            diff = _xpu_end - _xpu_start;
        }
        return static_cast<float>(diff) / loop_time;
    }
};

template <typename PARAM_T, typename PLAN_T, typename KEY_T> class XPU2Autotuner {
public:
    // NEED IMPLEMENT
    virtual KEY_T param_to_key(const PARAM_T& param) = 0;
    virtual std::vector<PLAN_T> generate_choices(const PARAM_T& param) = 0;
    virtual bool set_key_from_str(const std::string& str, KEY_T& key) = 0;
    virtual std::string key_to_string(const KEY_T&) = 0;
    virtual bool set_plan_from_str(const std::string& str, PLAN_T& plan) = 0;

    // COMMON IMPL
    void insert_plan(PARAM_T param, const PLAN_T& plan) {
        auto key = param_to_key(param);
        _map[key] = plan;
    }

    bool find(PARAM_T param, PLAN_T& plan) {
        auto key = param_to_key(param);
        auto it = _map.find(key);
        if (it != _map.end()) {
            plan = it->second;
            return true;
        }
        return false;
    }

    bool autotune(Context* ctx, PARAM_T param, int (*autotune_kernel_wrapper)(Context*, PARAM_T, PLAN_T),
                      PLAN_T& result) {
        if (autotune_loop <= 0) {
            return false;
        }
        if (find(param, result)) {
            return true;
        }
        int ret = 0;
        ret = autotune_tune(ctx, param, autotune_kernel_wrapper, result);
        WRAPPER_ASSERT_SUCCESS(ctx, ret);
        insert_plan(param, result);
        save_autotune_file(); // save each case to avoid losing result when autotune failed
        return true;
    }

    bool kl3_autotune(Context* ctx, PARAM_T param, int (*autotune_kernel_wrapper)(Context*, PARAM_T, PLAN_T),
                      PLAN_T& result) {
        if (autotune_loop <= 0) {
            return false;
        }
        if (find(param, result)) {
            return true;
        }
        return false;
    }

    void set_autotune_param(int loop, int choice_cnt, bool writeback, const char* filename) {
        autotune_loop = loop;
        autotune_choice_cnt = choice_cnt;
        autotune_writeback = writeback;
        if (filename != nullptr) {
            if (autotune_filename != nullptr) {
                delete[] autotune_filename;
                autotune_filename = nullptr;
            }
            autotune_filename = new char[strlen(filename) + 1];
            memcpy((void*)autotune_filename, (const void*)filename, strlen(filename) + 1);
            load_autotune_file();
        }
    }

    XPU2Autotuner(const char* name, int loop, int choice_cnt, bool writeback, const char* filename)
        : autotune_choice_cnt(choice_cnt), autotune_loop(loop),
            autotune_writeback(writeback), autotune_filename(filename), _name(nullptr) {
        std::string sname(name); // TODO: clean code
        if (_name != nullptr) {
            delete[] _name;
            _name = nullptr;
        }
        _name = new char[sname.length() + 1];
        strcpy(const_cast<char*>(_name), sname.c_str());
    }

    virtual ~XPU2Autotuner() {
        if (autotune_filename != nullptr) {
            delete[] autotune_filename;
            autotune_filename = nullptr;
        }
        if (_name != nullptr) {
            delete[] _name;
            _name = nullptr;
        }
    }

protected:
    int autotune_choice_cnt;
    int autotune_loop;
    bool autotune_writeback;
    const char* autotune_filename;
    const char* _name;
    std::map<KEY_T, PLAN_T> _map;
    int autotune_looper(Context* ctx, int (*autotune_kernel_wrapper)(Context*, PARAM_T, PLAN_T),
                        std::vector<PLAN_T> choices, PARAM_T params, int autotune_loop, int& result,
                        int log_level = 1) {
        if (choices.size() == 1) {
            return 0;
        }
        int ret = 0;
        float best_avg = FLT_MAX;
        AutotuneTimer autotune_timer;
        int best_idx = 0;
        // create checkpoints for in-place changed data
        auto checkpoint_datas = params.checkpoint_data();
        ctx_guard RAII_GUARD(ctx);
        auto checkpoints = std::vector<const void*>();
        for (std::pair<const void*, int> pair : checkpoint_datas) {
            auto des = RAII_GUARD.alloc<char>(pair.second);
            WRAPPER_ASSERT_WORKSPACE(ctx, des);
            ret = api::copy<int8_t>(ctx, (int8_t*)pair.first, (int8_t*)des, pair.second);
            WRAPPER_ASSERT_SUCCESS(ctx, ret);
            checkpoints.push_back(des);
        }

        xpu_wait(ctx->xpu_stream);
        for (int choice_idx = 0; choice_idx < choices.size(); ++choice_idx) {
            if (log_level > 0) {
                std::cout << "choice: " << choices[choice_idx].to_string() << " \n";
            }
            // run kernels
            xpu_wait(ctx->xpu_stream);
            autotune_timer.start(ctx);
            for (int i = 0; i < autotune_loop; ++i) {
                ret = autotune_kernel_wrapper(ctx, params, choices[choice_idx]);
                WRAPPER_ASSERT_SUCCESS(ctx, ret);
            }
            xpu_wait(ctx->xpu_stream);
            autotune_timer.stop(ctx);

            // update best plan
            float duration = autotune_timer.duration(autotune_loop);
            if (log_level > 0) {
                std::cout << "choice idx: " << choice_idx << "/" << (choices.size() - 1)
                          << " consume(f): " << (duration) << std::endl;
            }
            if (duration < best_avg) {
                best_avg = duration;
                best_idx = choice_idx;
            }
        }
        // restore checkpoints
        for (int i = 0; i < checkpoints.size(); ++i) {
            ret = api::copy<int8_t>(ctx, (int8_t*)checkpoints[i], (int8_t*)checkpoint_datas[i].first,
                                    checkpoint_datas[i].second);
            WRAPPER_ASSERT_SUCCESS(ctx, ret);
        }
        result = best_idx;
        return api::SUCCESS;
    }

    int autotune_tune(Context* ctx, const PARAM_T params, int (*autotune_kernel_wrapper)(Context*, PARAM_T, PLAN_T),
                          PLAN_T& result) {
        std::vector<PLAN_T> choices = generate_choices(params);
        std::cout << "autotune_gen_choices get choices: {" << choices.size() << "}" << std::endl;

        int best_idx = 0;
        int ret = 0;
        ret = autotune_looper(ctx, autotune_kernel_wrapper, choices, params, autotune_loop, best_idx);
        WRAPPER_ASSERT_SUCCESS(ctx, ret);

        std::cout << "autotune pick best_idx: " << best_idx << " plan: " << choices[best_idx].to_string() << std::endl;
        // TODO data holder
        result = choices[best_idx];
        return api::SUCCESS;
    }

    template <typename KEY, typename PLAN>
    int write_to_file(const std::string& fname, const std::map<KEY, PLAN>& plan_kv) {
        std::ofstream outfile(fname.c_str());
        int kv_cnt = 0;
        for (auto kv : plan_kv) {
            outfile << key_to_string(kv.first) << std::endl;
            outfile << kv.second.to_string() << std::endl;
            kv_cnt++;
        }
        outfile.close();
        return kv_cnt;
    }
    void save_autotune_file() {
        if (!autotune_writeback) {
            return;
        }
        int cnt = XPU2Autotuner::write_to_file<KEY_T, PLAN_T>(std::string(autotune_filename) + "_" + std::string(_name),
                                                              _map);
        if (cnt > 0) {
            std::cout << "XPU2 autotune_file " << _name << "{" << autotune_filename << "}"
                      << " save " << cnt << " records." << std::endl;
        }
    }

    int load_autotune_file() {
        std::string fname(autotune_filename);
        fname += "_";
        fname += _name;
        std::ifstream infile(fname);
        if (!infile) {
            std::cout << "autotune filename \"" << fname << "\" not exist, skip load." << std::endl;
            return -1;
        }
        std::string line;
        int kv_cnt = 0;
        while (getline(infile, line)) {
            KEY_T key;
            PLAN_T plan;
            if (!set_key_from_str(line, key)) {
                std::cout << "Key in file {" << fname << "} does not match: " << line << ".\n";
                std::cout << "Redo autotune if XPU_CONV_AUTOTUNE is set. Otherwise, use model-based planner instead.\n";
                break;
            }
            getline(infile, line);
            if (!set_plan_from_str(line, plan)) {
                std::cout << "Plan in file {" << fname << "} does not match: " << line << ".\n";
                std::cout << "Redo autotune if XPU_CONV_AUTOTUNE is set. Otherwise, use model-based planner instead.\n";
                break;
            }
            _map[key] = plan;
            kv_cnt++;
        }
        infile.close();

        if (kv_cnt > 0) {
            std::cout << "XPU2 autotune_file " << _name << "{" << autotune_filename << "}";
            std::cout << " set " << kv_cnt << " records." << std::endl;
        }
        return 0;
    }
};
} // namespace api
} // namespace xpu
} // namespace baidu
#endif
