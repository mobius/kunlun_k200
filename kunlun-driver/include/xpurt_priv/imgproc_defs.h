#ifndef BAIDU_XPU_RUNTIME_INCLUDE_IMAGE_PROC_DEFS_H
#define BAIDU_XPU_RUNTIME_INCLUDE_IMAGE_PROC_DEFS_H

// KL2 soc has 6 hardware Image Process(img_proc) instances
#define IMGPROC_MAX_CORES (6)

//SYSCON reg
#define VIDEO_RST_CTRL             (0x00D8)

#define IMG_PROC_FORCE_UPD         (0x0004)
//#define IMG_PROC_AUTO_UPD          (0x0008)
#define IMG_PROC_INT_MASK          (0x000C)
#define IMG_PROC_INT_CLEAR         (0x0010)
#define IMG_PROC_INT_STATUS_RAW    (0x0014)
#define IMG_PROC_INT_STATUS_MASKED (0x0018)
#define IMG_RDMA_CTRL              (0x0400)
#define IMG_RDMA_YUV_SIZE          (0x0404)
#define IMG_RDMA_TIMER_TH          (0x042C)
#define IMG_RDMA_START             (0x0450)
#define IMG_WDMA_RGB_SIZE          (0x0C04)

#define IMGPROC_RDMA_REGS_NUM    21
#define IMGPROC_RESIZE_REGS_NUM  6
#define IMGPROC_WDMA_REGS_NUM    15
#define IMGPROC_REGS_NUM         (IMGPROC_RDMA_REGS_NUM\
                                 + IMGPROC_RESIZE_REGS_NUM\
                                 + IMGPROC_WDMA_REGS_NUM)   /* total regs */

#define IMGPROC_TOP_REGS_BASE (0x0000)
#define IMGPROC_RDMA_REGS_BASE (0x0400)
#define IMGPROC_RESIZE_REGS_BASE (0x0800)
#define IMGPROC_WDMA_REGS_BASE (0x0c00)


#endif
