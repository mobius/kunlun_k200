###########################################################################
# Kbuild fragment for kunlun_peermem.ko
###########################################################################

#
# Define KUNLUN_PEERMEM_{SOURCES,OBJECTS}
#

KUNLUN_PEERMEM_SOURCES =
KUNLUN_PEERMEM_SOURCES += kunlun_peermem/kunlun_peermem.c

KUNLUN_PEERMEM_OBJECTS = $(patsubst %.c,%.o,$(KUNLUN_PEERMEM_SOURCES))

obj-m += kunlun_peermem.o
kunlun_peermem-y := $(KUNLUN_PEERMEM_OBJECTS)

KUNLUN_PEERMEM_KO = kunlun_peermem/kunlun_peermem.ko

NV_KERNEL_MODULE_TARGETS += $(KUNLUN_PEERMEM_KO)

#
# Define kunlun_peermem.ko-specific CFLAGS.
#
KUNLUN_PEERMEM_CFLAGS += -I$(src)/kunlun_peermem
KUNLUN_PEERMEM_CFLAGS += -UDEBUG -U_DEBUG -DNDEBUG -DNV_BUILD_MODULE_INSTANCES=0

#
# In case of MOFED installation, kunlun_peermem compilation
# needs paths to the MOFED headers in CFLAGS.
# MOFED's Module.symvers is needed for the build
# to find the additional ib_* symbols.
#
OFA_DIR := /usr/src/ofa_kernel
MLNX_OFED_KERNEL := $(shell ( test -d $(OFA_DIR)/$(KERNELRELEASE) && \
                              echo $(OFA_DIR)/$(KERNELRELEASE) ) || \
                      ( test -d $(OFA_DIR)/$(shell uname -m)/$(shell uname -r) && \
                        echo $(OFA_DIR)/$(shell uname -m)/$(shell uname -r)) || \
                      ( test -d $(OFA_DIR)/default && echo $(OFA_DIR)/default ) || \
                      ( test -d /var/lib/dkms/mlnx-ofed-kernel && \
                       ls -d /var/lib/dkms/mlnx-ofed-kernel/*/build ) || \
                      ( echo $(OFA_DIR) ))
ifneq ($(shell test -d $(MLNX_OFED_KERNEL) && echo "true" || echo "" ),)
    KUNLUN_PEERMEM_CFLAGS += -I$(MLNX_OFED_KERNEL)/include -I$(MLNX_OFED_KERNEL)/include/rdma
    KBUILD_EXTRA_SYMBOLS := $(MLNX_OFED_KERNEL)/Module.symvers

    # XXX(miaotianxiang): 某些os环境中可能需要手动指定ofed符号路径
    #KBUILD_EXTRA_SYMBOLS := /usr/src/ofa_kernel/x86_64/5.4.0-145-generic/Module.symvers
endif

$(call ASSIGN_PER_OBJ_CFLAGS, $(KUNLUN_PEERMEM_OBJECTS), $(KUNLUN_PEERMEM_CFLAGS))

#
# Register the conftests needed by kunlun_peermem.ko
#

NV_OBJECTS_DEPEND_ON_CONFTEST += $(KUNLUN_PEERMEM_OBJECTS)

NV_CONFTEST_GENERIC_COMPILE_TESTS += ib_peer_memory_symbols

NV_CONFTEST_FUNCTION_COMPILE_TESTS +=

NV_CONFTEST_TYPE_COMPILE_TESTS +=
