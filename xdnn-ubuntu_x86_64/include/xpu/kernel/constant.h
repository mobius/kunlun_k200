#ifndef __XPU_KERNEL_CONSTANT_H
#define __XPU_KERNEL_CONSTANT_H

#ifdef __xpu__

const int sizeof_fp32 = 4; // avoid using sizeof(float) directly; the result is treated as unsigned int
const int sizeof_int16 = 2;
const int sizeof_int8 = 1;

#endif // __xpu__
#endif // __XPU_KERNEL_CONSTANT_H
