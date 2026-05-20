/*
 * SPDX-FileCopyrightText: Copyright (c) 2021-2022 KUNLUNXIN CORPORATION & AFFILIATES. All rights reserved.
 * SPDX-License-Identifier: MIT
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "kl2/kl2.h"

/* KL2 Mailbox for SR-IOV
 *
 * Brief description:
 *  The file mbox.c implements a simple mailbox for comunication between
 *  PF and VF when enable KL2 SR-IOV. VF may use this mailbox to notify
 *  PF to do some high-privileged operations(such as reset physical
 *  cluster/sdnn, get physical hwq id map info, etc).
 *
 * Hardware depends:
 *  Depend on 3 registers from kl2 hardware mailbox0(DR0, DR1, DR2).
 *      - 3 32bit register(DR0~2) used for VF0,1,2 exchanging message with PF.
 *  More hardware info about these registers can be seen DDI0306.pdf in:
 *      http://wiki.baidu.com/display/ISA/IPC
 *
 * Message flow:
 *  1. Init mailbox(Enable hw data register and enable PF message handle timer).
 *  2. VF construct and write message to VF-related data register.
 *  3. VF write mailbox and poll response(if need).
 *  4. Handle message in PF if mailbox is not empty then write response back to VF.
 *  5. VF got response or timeout then clear data register.
 *
 * Message definition:
 *  type[31:28] | user_idx[27:24] | operation param[23:0]
 *     type: 0 <No Message>
 *         type[31:28] <0x0>
 *     type: 1 <vf cluster HWQ ID request>
 *         type[31:28] <0x1> | user_idx[27:24] | vf_cluster_idx[23:20] | hwq_id[19:16] | ack[3:0]
 *     type: 2 <vf sdnn HWQ ID request>
 *         type[31:28] <0x2> | user_idx[27:24] | vf_sdnn_idx[23:20] | hwq_id[19:16] | ack[3:0]
 *     type: 3 <vf cluster reset>
 *         type[31:28] <0x3> | user_idx[27:24] | vf_cluster_idx[23:20] | ack[3:0]
 *     type: 4 <vf sdnn reset>
 *         type[31:28] <0x4> | user_idx[27:24] | vf_sdnn_idx[23:20] | ack[3:0]
 *     type: 5 <vf xpu busy status request>
 *         type[31:28] <0x5> | user_idx[27:24] | sdnn_busy_status[17:12] |
 *                                               cluster busy_status[11:4] | ack[3:0]
 */

enum KL2_SRIOV_MSGTYPE {
    KL2_SRIOV_MSGTYPE_NONE                = 0,
    KL2_SRIOV_MSGTYPE_REQ_CLUSTER_HWQ_ID  = 1,
    KL2_SRIOV_MSGTYPE_REQ_SDNN_HWQ_ID     = 2,
    KL2_SRIOV_MSGTYPE_RESET_CLUSTER       = 3,
    KL2_SRIOV_MSGTYPE_RESET_SDNN          = 4,
    KL2_SRIOV_MSGTYPE_REQ_XPU_BUSY_STATUS = 5,
};

enum KL2_SRIOV_MBOX_STATUS {
    KL2_SRIOV_MBOX_STATUS_EMPTY     = 0,
    KL2_SRIOV_MBOX_STATUS_MSG_NEW   = 1,
    KL2_SRIOV_MBOX_STATUS_MSG_ACKED = 2,
};

#define KL2_SRIOV_MBOX_GET_MSG_TYPE(msg) (((msg)&0xf0000000) >> 28)
#define KL2_SRIOV_MBOX_GET_SRC_USER_ID(msg) (((msg)&0xf000000) >> 24)
#define KL2_SRIOV_MBOX_GET_SDNN_ID(msg) (((msg)&0xf00000) >> 20)
#define KL2_SRIOV_MBOX_GET_CLUSTER_ID(msg) (((msg)&0xf0000) >> 16)
#define KL2_SRIOV_MBOX_GET_HWQ_ID(msg) (((msg)&0xf000) >> 12)
#define KL2_SRIOV_MBOX_GET_XPU_BUSY_STATUS(msg) (((msg)&0x3fff0) >> 4)
#define KL2_SRIOV_MBOX_GET_STATUS(msg) ((msg)&0xf)

#define KL2_SRIOV_MBOX_SET_MSG_TYPE(msg, val) ((msg) |= ((val) << 28))
#define KL2_SRIOV_MBOX_SET_SRC_USER_ID(msg, val) ((msg) |= ((val) << 24))
#define KL2_SRIOV_MBOX_SET_SDNN_ID(msg, val) ((msg) |= ((val) << 20))
#define KL2_SRIOV_MBOX_SET_CLUSTER_ID(msg, val) ((msg) |= ((val) << 16))
#define KL2_SRIOV_MBOX_SET_HWQ_ID(msg, val) ((msg) |= ((val) << 12))
#define KL2_SRIOV_MBOX_SET_XPU_BUSY_STATUS(msg, val) ((msg) |= ((val) << 4))
#define KL2_SRIOV_MBOX_SET_STATUS(msg, val) ((msg) = (((msg)&0xfffffff0) | ((val)&0xf)))

int kl2_sriov_mbox_reset_cluster_vf(struct kl2_device *kl2_dev, int user_id, int vf_cluster_id,
                                    int timeout_ms);
int kl2_sriov_mbox_reset_sdnn_vf(struct kl2_device *kl2_dev, int user_id, int vf_sdnn_id,
                                 int timeout_ms);
int kl2_sriov_mbox_request_cluster_hwqid_vf(struct kl2_device *kl2_dev, int user_id,
                                            int vf_cluster_id, int timeout_ms);
int kl2_sriov_mbox_request_sdnn_hwqid_vf(struct kl2_device *kl2_dev, int user_id, int vf_sdnn_id,
                                         int timeout_ms);
int kl2_sriov_mbox_req_xpu_busy_status_vf(struct kl2_device *kl2_dev, int user_id,
                                          u32 *xpu_busy_status);

void kl2_sriov_mbox_init(struct kl2_device *kl2_dev);
void kl2_sriov_mbox_enable_pf(struct kl2_device *kl2_dev);
void kl2_sriov_mbox_disable_pf(struct kl2_device *kl2_dev);
