
#ifndef SOFTWARE_LINUX_DWL_VDEC_DWL_DEFS_H_
#define SOFTWARE_LINUX_DWL_VDEC_DWL_DEFS_H_

#define DWL_MPEG2_E      31 /* 1 bit  */
#define DWL_VC1_E        29 /* 2 bits */
#define DWL_JPEG_E       28 /* 1 bit  */
#define DWL_HJPEG_E      17 /* 1 bit  */
#define DWL_MPEG4_E      26 /* 2 bits */
#define DWL_H264_E       24 /* 2 bits */
#define DWL_H264HIGH10_E 20 /* 1 bits */
#define DWL_AVS2_E       18 /* 2 bits */
#define DWL_VP6_E        23 /* 1 bit  */
#define DWL_RV_E         26 /* 2 bits */
#define DWL_VP8_E        23 /* 1 bit  */
#define DWL_VP7_E        24 /* 1 bit  */
#define DWL_WEBP_E       19 /* 1 bit  */
#define DWL_AVS_E        22 /* 1 bit  */
#define DWL_G1_PP_E      16 /* 1 bit  */
#define DWL_G2_PP_E      31 /* 1 bit  */
#define DWL_PP_E         31 /* 1 bit  */
#define DWL_HEVC_E       26 /* 3 bits */
#define DWL_VP9_E        29 /* 3 bits */

#define DWL_H264_PIPELINE_E 31 /* 1 bit */
#define DWL_JPEG_PIPELINE_E 30 /* 1 bit */

#define DWL_G2_HEVC_E    0  /* 1 bits */
#define DWL_G2_VP9_E     1  /* 1 bits */
#define DWL_G2_RFC_E        2  /* 1 bits */
#define DWL_RFC_E        17  /* 2 bits */
#define DWL_G2_DS_E         3  /* 1 bits */
#define DWL_DS_E         28  /* 3 bits */
#define DWL_HEVC_VER     8  /* 4 bits */
#define DWL_VP9_PROFILE  12 /* 3 bits */
#define DWL_RING_E       16 /* 1 bits */

#define VDEC_IRQ_STAT_DEC       1
#define VDEC_IRQ_STAT_DEC_OFF   (VDEC_IRQ_STAT_DEC * 4)

#define VDECPP_SYNTH_CFG        60
#define VDECPP_SYNTH_CFG_OFF    (VDECPP_SYNTH_CFG * 4)
#define VDEC_SYNTH_CFG          50
#define VDEC_SYNTH_CFG_OFF      (VDEC_SYNTH_CFG * 4)
#define VDEC_SYNTH_CFG_2        54
#define VDEC_SYNTH_CFG_2_OFF    (VDEC_SYNTH_CFG_2 * 4)
#define VDEC_SYNTH_CFG_3        56
#define VDEC_SYNTH_CFG_3_OFF    (VDEC_SYNTH_CFG_3 * 4)
#define VDEC_CFG_STAT           23
#define VDEC_CFG_STAT_OFF       (VDEC_CFG_STAT * 4)
#define VDECPP_CFG_STAT         260
#define VDECPP_CFG_STAT_OFF     (VDECPP_CFG_STAT * 4)

#define VDEC_DEC_E              0x01
#define VDEC_PP_E               0x01
#define VDEC_DEC_ABORT          0x20
#define VDEC_DEC_IRQ_DISABLE    0x10
#define VDEC_DEC_IRQ            0x100

/* VC8000D HW build id */
#define VDEC_HW_BUILD_ID        309
#define VDEC_HW_BUILD_ID_OFF    (VDEC_HW_BUILD_ID * 4)

#define VDEC_DEC_E                 0x01
#define VDEC_PP_E                  0x01
#define VDEC_DEC_ABORT             0x20
#define VDEC_DEC_IRQ_DISABLE       0x10
#define VDEC_PP_IRQ_DISABLE        0x10
#define VDEC_DEC_IRQ               0x100
#define VDEC_PP_IRQ                0x100

#endif /* SOFTWARE_LINUX_DWL_VDEC_DWL_DEFS_H_ */
