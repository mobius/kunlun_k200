#ifndef BAIDU_XPU_RUNTIME_INCLUDE_XPU_DEFS_PRIVATE_KL1_H
#define BAIDU_XPU_RUNTIME_INCLUDE_XPU_DEFS_PRIVATE_KL1_H

#include "xpurt_priv/defs_private.h"

enum {
    // iATU
    KL1_INBOUND_REGION_NUM  = 4,
    KL1_OUTBOUND_REGION_NUM = 4,
    KL1_MAX_REGION_NUM      = 4, // max(KL1_INBOUND_REGION_NUM, KL1_OUTBOUND_REGION_NUM)
};

struct iatu_region_info {
    int      region_type;
    int      region_en;
    int      match_mode[KL1_MAX_REGION_NUM];
    uint32_t bar[KL1_MAX_REGION_NUM];
    uint64_t size[KL1_MAX_REGION_NUM];
    uint64_t base[KL1_MAX_REGION_NUM];
    uint64_t target[KL1_MAX_REGION_NUM];
};

#endif
