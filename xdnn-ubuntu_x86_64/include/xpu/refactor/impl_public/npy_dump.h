#ifndef BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_NPY_DUMP_H
#define BAIDU_XPU_API_INCLUDE_XPU_REFACTOR_IMPL_NPY_DUMP_H
#include <string>
#include <cstring>
#include <vector>
#include "xpu/dll_export.h"
namespace npy {
/* npy array length */
typedef unsigned long int ndarray_len_t;

template<typename Scalar>
DLL_EXPORT void SaveArrayAsNumpy(const std::string& filename, bool fortran_order, const std::vector<ndarray_len_t>& shape_v,
        const Scalar* data);
template<typename Scalar>
DLL_EXPORT void SaveArrayAsNumpy(const std::string& filename, bool fortran_order, const size_t shape_len,
        const Scalar* data);

template<typename Scalar>
DLL_EXPORT void SaveArrayAsNumpy(const std::string& filename, bool fortran_order, const std::vector<ndarray_len_t>& shape_v,
        const std::vector <Scalar>& data);
template<typename Scalar>
DLL_EXPORT void SaveArrayAsNumpy(const std::string& filename, bool fortran_order, const size_t shape_len,
        const std::vector <Scalar>& data);

template<typename Scalar>
DLL_EXPORT void LoadArrayFromNumpy(const std::string& filename, std::vector<unsigned long>& shape, std::vector <Scalar>& data);

template<typename Scalar>
DLL_EXPORT void LoadArrayFromNumpy(const std::string& filename, std::vector<unsigned long>& shape, bool& fortran_order,
        std::vector <Scalar>& data);

}

#endif
