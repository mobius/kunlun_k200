#ifndef BAIDU_XPU_RUNTIME_MODULE_KL2_IOCTL_IMGPROC_H_
#define BAIDU_XPU_RUNTIME_MODULE_KL2_IOCTL_IMGPROC_H_

#include <linux/ioctl.h>
#include "xpu/defs.h"

#define IOCTL_IMGPROC_MAGIC    'p'

struct imgproc_ioctl_run_param {
    int      core_id;
    uint32_t *regs;
};

struct imgproc_ioctl_rw_reg_param {
    int      core_id;
    uint32_t reg_offset;
    uint32_t data;
};

// status code for IOCTL_IMGPROC_WAIT
typedef enum {
    IMGPROC_WAIT_OK,
    IMGPROC_WAIT_TIME_OUT,
    IMGPROC_WAIT_ERR,
} wait_status_t;

#define IOCTL_IMGPROC_RESET       _IO(IOCTL_IMGPROC_MAGIC, 1)
#define IOCTL_IMGPROC_CORE_NUM    _IO(IOCTL_IMGPROC_MAGIC, 2)
#define IOCTL_IMGPROC_RESERVE     _IO(IOCTL_IMGPROC_MAGIC, 3)
#define IOCTL_IMGPROC_RELEASE     _IO(IOCTL_IMGPROC_MAGIC, 4)
#define IOCTL_IMGPROC_RUN         _IOW(IOCTL_IMGPROC_MAGIC, 5, struct imgproc_ioctl_run_param *)
#define IOCTL_IMGPROC_WAIT        _IO(IOCTL_IMGPROC_MAGIC, 6)
#define IOCTL_IMGPROC_WRITE_REG   _IOW(IOCTL_IMGPROC_MAGIC,  7, struct imgproc_ioctl_rw_reg_param *)
#define IOCTL_IMGPROC_READ_REG    _IOWR(IOCTL_IMGPROC_MAGIC, 8, struct imgproc_ioctl_rw_reg_param *)
#define IOCTL_IMGPROC_MAXNR        (8)
#endif //BAIDU_XPU_RUNTIME_MODULE_KL2_IOCTL_IMGPROC_H_
