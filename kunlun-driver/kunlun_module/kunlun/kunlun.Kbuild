###########################################################################
# Kbuild fragment for kunlun.ko
###########################################################################

#
# Define KUNLUN_{SOURCES,OBJECTS}
#

include $(src)/kunlun/kunlun-sources.Kbuild
KUNLUN_OBJECTS = $(patsubst %.c,%.o,$(KUNLUN_SOURCES))

obj-m += kunlun.o
kunlun-y := $(KUNLUN_OBJECTS)

KUNLUN_KO = kunlun/kunlun.ko


#
# nv-kernel.o_binary is the core binary component of kunlun.ko, shared
# across all UNIX platforms. Create a symlink, "nv-kernel.o" that
# points to nv-kernel.o_binary, and add nv-kernel.o to the list of
# objects to link into kunlun.ko.
#
# Note that:
# - The kbuild "clean" rule will delete all objects in kunlun-y (which
# is why we use a symlink instead of just adding nv-kernel.o_binary
# to kunlun-y).
# - kbuild normally uses the naming convention of ".o_shipped" for
# binary files. That is not used here, because the kbuild rule to
# create the "normal" object file from ".o_shipped" does a copy, not
# a symlink. This file is quite large, so a symlink is preferred.
# - The file added to kunlun-y should be relative to gmake's cwd.
# But, the target for the symlink rule should be prepended with $(obj).
# - The "symlink" command is called using kbuild's if_changed macro to
# generate an .nv-kernel.o.cmd file which can be used on subsequent
# runs to determine if the command line to create the symlink changed
# and needs to be re-executed.
#
#
#NVIDIA_BINARY_OBJECT := $(src)/nvidia/nv-kernel.o_binary
#NVIDIA_BINARY_OBJECT_O := nvidia/nv-kernel.o
#
#quiet_cmd_symlink = SYMLINK $@
# cmd_symlink = ln -sf $< $@
#
#targets += $(NVIDIA_BINARY_OBJECT_O)
#
#$(obj)/$(NVIDIA_BINARY_OBJECT_O): $(NVIDIA_BINARY_OBJECT) FORCE
#	$(call if_changed,symlink)
#
#nvidia-y += $(NVIDIA_BINARY_OBJECT_O)


#
# Define kunlun.ko-specific CFLAGS.
#

KUNLUN_CFLAGS += -I$(src)/kunlun
KUNLUN_CFLAGS += -DNVIDIA_UNDEF_LEGACY_BIT_MACROS
KUNLUN_CFLAGS += -UDEBUG -U_DEBUG -DNDEBUG
# 开启KL2_LOGD等
#KUNLUN_CFLAGS += -DDEBUG

# XXX(miaotianxiang): 此处配置驱动行为相关变量和宏定义
TARGET_PLATFORM = kunlun
ENABLE_CODEC = 1
ENABLE_RB = 0

ifeq ($(TARGET_PLATFORM), pld)
KUNLUN_CFLAGS += -DPLATFORM_PLD
else ifeq ($(TARGET_PLATFORM), zebu)
KUNLUN_CFLAGS += -DPLATFORM_ZEBU
else ifeq ($(TARGET_PLATFORM), kunlun)
KUNLUN_CFLAGS += -DPLATFORM_KUNLUN
else
KUNLUN_CFLAGS += -DPLATFORM_KUNLUN
endif

ifeq ($(ENABLE_CODEC), 1)
KUNLUN_CFLAGS += -DENABLE_CODEC
endif

ifeq ($(ENABLE_RB), 1)
KUNLUN_CFLAGS += -DENABLE_RB
endif

$(call ASSIGN_PER_OBJ_CFLAGS, $(KUNLUN_OBJECTS), $(KUNLUN_CFLAGS))


#
# nv-procfs.c requires nv-compiler.h
#

NV_COMPILER_VERSION_HEADER = $(obj)/kunlun/port/nv_compiler.h

$(NV_COMPILER_VERSION_HEADER):
	@echo \#define NV_COMPILER \"`$(CC) -v 2>&1 | tail -n 1`\" > $@

$(obj)/kunlun/port/nv-procfs.o: $(NV_COMPILER_VERSION_HEADER)

clean-files += $(NV_COMPILER_VERSION_HEADER)


#
# Build kl-interface.o from the kernel interface layer objects, suitable
# for further processing by the top-level makefile to produce a precompiled
# kernel interface file.
#

KUNLUN_INTERFACE := kunlun/kl-interface.o

# Linux kernel v5.12 and later looks at "always-y", Linux kernel versions
# before v5.6 looks at "always"; kernel versions between v5.12 and v5.6
# look at both.

always += $(KUNLUN_INTERFACE)
always-y += $(KUNLUN_INTERFACE)

$(obj)/$(KUNLUN_INTERFACE): $(addprefix $(obj)/,$(KUNLUN_OBJECTS))
	$(LD) -r -o $@ $^


#
# Register the conftests needed by kunlun.ko
#

NV_OBJECTS_DEPEND_ON_CONFTEST += $(KUNLUN_OBJECTS)

NV_CONFTEST_FUNCTION_COMPILE_TESTS += hash__remap_4k_pfn
NV_CONFTEST_FUNCTION_COMPILE_TESTS += set_pages_uc
NV_CONFTEST_FUNCTION_COMPILE_TESTS += list_is_first
NV_CONFTEST_FUNCTION_COMPILE_TESTS += set_memory_uc
NV_CONFTEST_FUNCTION_COMPILE_TESTS += set_memory_array_uc
NV_CONFTEST_FUNCTION_COMPILE_TESTS += set_pages_array_uc
NV_CONFTEST_FUNCTION_COMPILE_TESTS += acquire_console_sem
NV_CONFTEST_FUNCTION_COMPILE_TESTS += console_lock
NV_CONFTEST_FUNCTION_COMPILE_TESTS += ioremap_cache
NV_CONFTEST_FUNCTION_COMPILE_TESTS += ioremap_wc
NV_CONFTEST_FUNCTION_COMPILE_TESTS += acpi_walk_namespace
NV_CONFTEST_FUNCTION_COMPILE_TESTS += sg_alloc_table
NV_CONFTEST_FUNCTION_COMPILE_TESTS += pci_get_domain_bus_and_slot
NV_CONFTEST_FUNCTION_COMPILE_TESTS += get_num_physpages
NV_CONFTEST_FUNCTION_COMPILE_TESTS += efi_enabled
NV_CONFTEST_FUNCTION_COMPILE_TESTS += pde_data
NV_CONFTEST_FUNCTION_COMPILE_TESTS += PDE_DATA
NV_CONFTEST_FUNCTION_COMPILE_TESTS += proc_remove
NV_CONFTEST_FUNCTION_COMPILE_TESTS += pm_vt_switch_required
NV_CONFTEST_FUNCTION_COMPILE_TESTS += xen_ioemu_inject_msi
NV_CONFTEST_FUNCTION_COMPILE_TESTS += phys_to_dma
NV_CONFTEST_FUNCTION_COMPILE_TESTS += get_dma_ops
NV_CONFTEST_FUNCTION_COMPILE_TESTS += dma_attr_macros
NV_CONFTEST_FUNCTION_COMPILE_TESTS += dma_map_page_attrs
NV_CONFTEST_FUNCTION_COMPILE_TESTS += write_cr4
NV_CONFTEST_FUNCTION_COMPILE_TESTS += of_get_property
NV_CONFTEST_FUNCTION_COMPILE_TESTS += of_find_node_by_phandle
NV_CONFTEST_FUNCTION_COMPILE_TESTS += of_node_to_nid
NV_CONFTEST_FUNCTION_COMPILE_TESTS += pnv_pci_get_npu_dev
NV_CONFTEST_FUNCTION_COMPILE_TESTS += of_get_ibm_chip_id
NV_CONFTEST_FUNCTION_COMPILE_TESTS += node_end_pfn
NV_CONFTEST_FUNCTION_COMPILE_TESTS += pci_bus_address
NV_CONFTEST_FUNCTION_COMPILE_TESTS += pci_stop_and_remove_bus_device
NV_CONFTEST_FUNCTION_COMPILE_TESTS += pci_remove_bus_device
NV_CONFTEST_FUNCTION_COMPILE_TESTS += register_cpu_notifier
NV_CONFTEST_FUNCTION_COMPILE_TESTS += cpuhp_setup_state
NV_CONFTEST_FUNCTION_COMPILE_TESTS += dma_map_resource
NV_CONFTEST_FUNCTION_COMPILE_TESTS += backlight_device_register
NV_CONFTEST_FUNCTION_COMPILE_TESTS += get_backlight_device_by_name
NV_CONFTEST_FUNCTION_COMPILE_TESTS += timer_setup
NV_CONFTEST_FUNCTION_COMPILE_TESTS += pci_enable_msix_range
NV_CONFTEST_FUNCTION_COMPILE_TESTS += kernel_read_has_pointer_pos_arg
NV_CONFTEST_FUNCTION_COMPILE_TESTS += kernel_write
NV_CONFTEST_FUNCTION_COMPILE_TESTS += kthread_create_on_node
NV_CONFTEST_FUNCTION_COMPILE_TESTS += of_find_matching_node
NV_CONFTEST_FUNCTION_COMPILE_TESTS += dev_is_pci
NV_CONFTEST_FUNCTION_COMPILE_TESTS += dma_direct_map_resource
NV_CONFTEST_FUNCTION_COMPILE_TESTS += tegra_get_platform
NV_CONFTEST_FUNCTION_COMPILE_TESTS += tegra_bpmp_send_receive
NV_CONFTEST_FUNCTION_COMPILE_TESTS += flush_cache_all
NV_CONFTEST_FUNCTION_COMPILE_TESTS += vmf_insert_pfn
NV_CONFTEST_FUNCTION_COMPILE_TESTS += jiffies_to_timespec
NV_CONFTEST_FUNCTION_COMPILE_TESTS += ktime_get_raw_ts64
NV_CONFTEST_FUNCTION_COMPILE_TESTS += ktime_get_real_ts64
NV_CONFTEST_FUNCTION_COMPILE_TESTS += full_name_hash
NV_CONFTEST_FUNCTION_COMPILE_TESTS += hlist_for_each_entry
NV_CONFTEST_FUNCTION_COMPILE_TESTS += pci_enable_atomic_ops_to_root
NV_CONFTEST_FUNCTION_COMPILE_TESTS += vga_tryget
NV_CONFTEST_FUNCTION_COMPILE_TESTS += pgprot_decrypted
NV_CONFTEST_FUNCTION_COMPILE_TESTS += iterate_fd
NV_CONFTEST_FUNCTION_COMPILE_TESTS += seq_read_iter
NV_CONFTEST_FUNCTION_COMPILE_TESTS += sg_page_iter_page
NV_CONFTEST_FUNCTION_COMPILE_TESTS += unsafe_follow_pfn
NV_CONFTEST_FUNCTION_COMPILE_TESTS += drm_gem_object_get
NV_CONFTEST_FUNCTION_COMPILE_TESTS += drm_gem_object_put_unlocked
NV_CONFTEST_FUNCTION_COMPILE_TESTS += set_close_on_exec
NV_CONFTEST_FUNCTION_COMPILE_TESTS += add_memory_driver_managed
NV_CONFTEST_FUNCTION_COMPILE_TESTS += device_property_read_u64
NV_CONFTEST_FUNCTION_COMPILE_TESTS += devm_of_platform_populate
NV_CONFTEST_FUNCTION_COMPILE_TESTS += of_dma_configure
NV_CONFTEST_FUNCTION_COMPILE_TESTS += of_property_count_elems_of_size
NV_CONFTEST_FUNCTION_COMPILE_TESTS += of_property_read_variable_u8_array
NV_CONFTEST_FUNCTION_COMPILE_TESTS += i2c_new_client_device
NV_CONFTEST_FUNCTION_COMPILE_TESTS += i2c_unregister_device
NV_CONFTEST_FUNCTION_COMPILE_TESTS += of_get_named_gpio
NV_CONFTEST_FUNCTION_COMPILE_TESTS += devm_gpio_request_one
NV_CONFTEST_FUNCTION_COMPILE_TESTS += gpio_direction_input
NV_CONFTEST_FUNCTION_COMPILE_TESTS += gpio_direction_output
NV_CONFTEST_FUNCTION_COMPILE_TESTS += gpio_get_value
NV_CONFTEST_FUNCTION_COMPILE_TESTS += gpio_set_value
NV_CONFTEST_FUNCTION_COMPILE_TESTS += gpio_to_irq
NV_CONFTEST_FUNCTION_COMPILE_TESTS += icc_get
NV_CONFTEST_FUNCTION_COMPILE_TESTS += icc_put
NV_CONFTEST_FUNCTION_COMPILE_TESTS += icc_set_bw
NV_CONFTEST_FUNCTION_COMPILE_TESTS += iommu_get_domain_for_dev
NV_CONFTEST_FUNCTION_COMPILE_TESTS += class_dev_uevent_has_const_dev_args

NV_CONFTEST_SYMBOL_COMPILE_TESTS += is_export_symbol_gpl_of_node_to_nid
NV_CONFTEST_SYMBOL_COMPILE_TESTS += is_export_symbol_gpl_sme_active
NV_CONFTEST_SYMBOL_COMPILE_TESTS += is_export_symbol_present_swiotlb_map_sg_attrs
NV_CONFTEST_SYMBOL_COMPILE_TESTS += is_export_symbol_present_swiotlb_dma_ops
NV_CONFTEST_SYMBOL_COMPILE_TESTS += is_export_symbol_present___close_fd
NV_CONFTEST_SYMBOL_COMPILE_TESTS += is_export_symbol_present_close_fd
NV_CONFTEST_SYMBOL_COMPILE_TESTS += is_export_symbol_present_get_unused_fd
NV_CONFTEST_SYMBOL_COMPILE_TESTS += is_export_symbol_present_get_unused_fd_flags
NV_CONFTEST_SYMBOL_COMPILE_TESTS += is_export_symbol_present_nvhost_get_default_device
NV_CONFTEST_SYMBOL_COMPILE_TESTS += is_export_symbol_present_nvhost_syncpt_unit_interface_get_byte_offset
NV_CONFTEST_SYMBOL_COMPILE_TESTS += is_export_symbol_present_nvhost_syncpt_unit_interface_get_aperture
NV_CONFTEST_SYMBOL_COMPILE_TESTS += is_export_symbol_present_tegra_dce_register_ipc_client
NV_CONFTEST_SYMBOL_COMPILE_TESTS += is_export_symbol_present_tegra_dce_unregister_ipc_client
NV_CONFTEST_SYMBOL_COMPILE_TESTS += is_export_symbol_present_tegra_dce_client_ipc_send_recv
NV_CONFTEST_SYMBOL_COMPILE_TESTS += is_export_symbol_present_dram_clk_to_mc_clk
NV_CONFTEST_SYMBOL_COMPILE_TESTS += is_export_symbol_present_get_dram_num_channels
NV_CONFTEST_SYMBOL_COMPILE_TESTS += is_export_symbol_present_tegra_dram_types

NV_CONFTEST_TYPE_COMPILE_TESTS += acpi_op_remove
NV_CONFTEST_TYPE_COMPILE_TESTS += file_operations
NV_CONFTEST_TYPE_COMPILE_TESTS += file_inode
NV_CONFTEST_TYPE_COMPILE_TESTS += kuid_t
NV_CONFTEST_TYPE_COMPILE_TESTS += dma_ops
NV_CONFTEST_TYPE_COMPILE_TESTS += swiotlb_dma_ops
NV_CONFTEST_TYPE_COMPILE_TESTS += noncoherent_swiotlb_dma_ops
NV_CONFTEST_TYPE_COMPILE_TESTS += vm_fault_has_address
NV_CONFTEST_TYPE_COMPILE_TESTS += backlight_properties_type
NV_CONFTEST_TYPE_COMPILE_TESTS += vm_insert_pfn_prot
NV_CONFTEST_TYPE_COMPILE_TESTS += vmf_insert_pfn_prot
NV_CONFTEST_TYPE_COMPILE_TESTS += address_space_init_once
NV_CONFTEST_TYPE_COMPILE_TESTS += vm_ops_fault_removed_vma_arg
NV_CONFTEST_TYPE_COMPILE_TESTS += vmbus_channel_has_ringbuffer_page
NV_CONFTEST_TYPE_COMPILE_TESTS += device_driver_of_match_table
NV_CONFTEST_TYPE_COMPILE_TESTS += device_of_node
NV_CONFTEST_TYPE_COMPILE_TESTS += node_states_n_memory
NV_CONFTEST_TYPE_COMPILE_TESTS += kmem_cache_has_kobj_remove_work
NV_CONFTEST_TYPE_COMPILE_TESTS += sysfs_slab_unlink
NV_CONFTEST_TYPE_COMPILE_TESTS += proc_ops
NV_CONFTEST_TYPE_COMPILE_TESTS += timespec64
NV_CONFTEST_TYPE_COMPILE_TESTS += vmalloc_has_pgprot_t_arg
NV_CONFTEST_TYPE_COMPILE_TESTS += acpi_fadt_low_power_s0
NV_CONFTEST_TYPE_COMPILE_TESTS += mm_has_mmap_lock
NV_CONFTEST_TYPE_COMPILE_TESTS += pci_channel_state
NV_CONFTEST_TYPE_COMPILE_TESTS += pci_dev_has_ats_enabled
NV_CONFTEST_TYPE_COMPILE_TESTS += mt_device_gre
NV_CONFTEST_TYPE_COMPILE_TESTS += remove_memory_has_nid_arg
NV_CONFTEST_TYPE_COMPILE_TESTS += iommu_map_has_gfp_arg
NV_CONFTEST_TYPE_COMPILE_TESTS += vm_area_struct_has_const_vm_flags
NV_CONFTEST_TYPE_COMPILE_TESTS += class_create_has_owner_arg


NV_CONFTEST_GENERIC_COMPILE_TESTS += dom0_kernel_present
NV_CONFTEST_GENERIC_COMPILE_TESTS += nvidia_vgpu_kvm_build
NV_CONFTEST_GENERIC_COMPILE_TESTS += nvidia_grid_build
NV_CONFTEST_GENERIC_COMPILE_TESTS += nvidia_grid_csp_build
NV_CONFTEST_GENERIC_COMPILE_TESTS += get_user_pages
NV_CONFTEST_GENERIC_COMPILE_TESTS += get_user_pages_remote
NV_CONFTEST_GENERIC_COMPILE_TESTS += pm_runtime_available
NV_CONFTEST_GENERIC_COMPILE_TESTS += vm_fault_t
NV_CONFTEST_GENERIC_COMPILE_TESTS += pci_class_multimedia_hd_audio
NV_CONFTEST_GENERIC_COMPILE_TESTS += drm_available

# vim: set ft=Makefile
