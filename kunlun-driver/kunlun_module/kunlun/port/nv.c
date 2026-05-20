/*
 * SPDX-FileCopyrightText: Copyright (c) 1999-2021 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include "nvmisc.h"
#include "os-interface.h"
#include "nv-linux.h"
//// #include "nv-p2p.h"
#include "nv-reg.h"
//// #include "nv-msi.h"
//// #include "nv-pci-table.h"
////
//// #if defined(NV_UVM_ENABLE)
//// #include "nv_uvm_interface.h"
//// #endif
////
//// #if defined(NV_VGPU_KVM_BUILD)
//// #include "nv-vgpu-vfio-interface.h"
//// #endif
////
////
//// #include "nvlink_proto.h"
//// #include "nvlink_caps.h"
////
////
//// #include "nv-frontend.h"
//// #include "nv-hypervisor.h"
//// #include "nv-ibmnpu.h"
//// #include "nv-rsync.h"
#include "nv-kthread-q.h"
//// #include "nv-pat.h"
////
//// #if !defined(CONFIG_RETPOLINE)
//// #include "nv-retpoline.h"
//// #endif
////
//// #include <linux/firmware.h>
////
//// #include <sound/core.h>             /* HDA struct snd_card */
////
//// #if defined(NV_SOUND_HDAUDIO_H_PRESENT)
//// #include "sound/hdaudio.h"
//// #endif
////
//// #if defined(NV_SOUND_HDA_CODEC_H_PRESENT)
//// #include <sound/core.h>
//// #include <sound/hda_codec.h>
//// #include <sound/hda_verbs.h>
//// #endif
////
//// #if defined(NV_SEQ_READ_ITER_PRESENT)
//// #include <linux/uio.h>
//// #include <linux/seq_file.h>
//// #include <linux/kernfs.h>
//// #endif
////
//// #include <linux/dmi.h>              /* System DMI info */
////
//// #include "conftest/patches.h"
////
//// #define RM_THRESHOLD_TOTAL_IRQ_COUNT     100000
//// #define RM_THRESHOLD_UNAHNDLED_IRQ_COUNT 99900
//// #define RM_UNHANDLED_TIMEOUT_US          100000
////
//// const NvBool nv_is_rm_firmware_supported_os = NV_TRUE;
////
//// // Deprecated, use NV_REG_ENABLE_GPU_FIRMWARE instead
//// char *rm_firmware_active = NULL;
//// NV_MODULE_STRING_PARAMETER(rm_firmware_active);
////
//// #define NV_FIRMWARE_GSP_FILENAME     "nvidia/" NV_VERSION_STRING "/gsp.bin"
//// #define NV_FIRMWARE_GSP_LOG_FILENAME "nvidia/" NV_VERSION_STRING "/gsp_log.bin"
////
//// MODULE_FIRMWARE(NV_FIRMWARE_GSP_FILENAME);
////
//// /*
////  * Global NVIDIA capability state, for GPU driver
////  */
//// nv_cap_t *nvidia_caps_root = NULL;
////
//// /*
////  * our global state; one per device
////  */
//// NvU32 num_nv_devices = 0;
//// NvU32 num_probed_nv_devices = 0;
////
//// nv_linux_state_t *nv_linux_devices;
////
//// /*
////  * And one for the control device
////  */
//// nv_linux_state_t nv_ctl_device = { { 0 } };
////
nv_kthread_q_t nv_kthread_q;
nv_kthread_q_t nv_deferred_close_kthread_q;
////
//// struct rw_semaphore nv_system_pm_lock;
////
//// #if defined(CONFIG_PM)
//// static nv_power_state_t nv_system_power_state;
//// static nv_pm_action_depth_t nv_system_pm_action_depth;
//// struct semaphore nv_system_power_state_lock;
//// #endif
////
//// void *nvidia_p2p_page_t_cache;
//// static void *nvidia_pte_t_cache;
//// void *nvidia_stack_t_cache;
//// static nvidia_stack_t *__nv_init_sp;
////
//// static int nv_tce_bypass_mode = NV_TCE_BYPASS_MODE_DEFAULT;
////
//// struct semaphore nv_linux_devices_lock;
////
//// static NvTristate nv_chipset_is_io_coherent = NV_TRISTATE_INDETERMINATE;

NvBool nvos_is_chipset_io_coherent(void)
{
    return NV_TRUE;
}
