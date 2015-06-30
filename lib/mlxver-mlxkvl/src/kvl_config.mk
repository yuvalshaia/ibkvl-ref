################################################################################
#
# Mellanox VL driver library Makefile
# Copyright(c) 2010 - 2011 Mellanox Technologies
#
# This program is free software; you can redistribute it and/or modify it
# under the terms and conditions of the GNU General Public License,
# version 2, as published by the Free Software Foundation.
#
# This program is distributed in the hope it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
# more details.
#
# You should have received a copy of the GNU General Public License along with
# this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
#
# The full GNU General Public License is included in this distribution in
# the file called "COPYING".
#
# Contact Information:
# 	SaeedM <saeedm@mellanox.com>
#
################################################################################
OFA_PREFIX = $(shell /etc/infiniband/info | grep prefix | cut -d= -f2)
ifneq ($(OFA_PREFIX),)
        $(warning *** Found OFED under prefix=$(OFA_PREFIX),)
        #find OFED kernel headers
        IS_OPENIB = $(shell test -d $(OFA_PREFIX)/src/openib && echo yes)
        ifeq ($(IS_OPENIB),yes)
                OFA_KERNEL=$(OFA_PREFIX)/src/ofa_kernel
        else
                OFA_KERNEL=$(OFA_PREFIX)/src/ofa_kernel/default
        endif
        $(warning *** OFA_KERNEL=$(OFA_KERNEL),)
        OFA_INCLUDES=-I$(OFA_KERNEL)/include/ -I$(OFA_KERNEL)/include/rdma 

        # OFED releated Defines
        QPT_XRC_EXIST = $(shell (grep -w IB_QPT_XRC $(OFA_KERNEL)/include/rdma/ib_verbs.h))
        QPT_RAW_ETHTYPE_EXIST = $(shell (grep -w IB_QPT_RAW_ETHERTYPE $(OFA_KERNEL)/include/rdma/ib_verbs.h))
        QPT_RAW_ETY_EXIST = $(shell (grep -w IB_QPT_RAW_ETY $(OFA_KERNEL)/include/rdma/ib_verbs.h))
        IB_EVENT_GID_CHANGE_EXIST = $(shell (grep -w IB_EVENT_GID_CHANGE $(OFA_KERNEL)/include/rdma/ib_verbs.h))
        ifneq ($(QPT_RAW_ETHTYPE_EXIST),)
                EXTRA_CFLAGS+=-D__QPT_ETHERTYPE_
        endif
        ifneq ($(QPT_RAW_ETY_EXIST),)
                EXTRA_CFLAGS+=-D__QPT_RAW_ETY_
        endif
        ifneq ($(IB_EVENT_GID_CHANGE_EXIST),)
                EXTRA_CFLAGS+=-D_IB_EVENT_GID_CHANGE_EXIST
        endif
        ifneq ($(QPT_XRC_EXIST),)
                EXTRA_CFLAGS+=-D__QPT_XRC_
        endif
        EXTRA_CFLAGS += -DIBDRIVER_EXIST
        # OFA BackPorts
        IS_OFA_KERNEL_MK_EXIST=$(shell test -f $(OFA_KERNEL)/config.mk && echo yes)
        ifeq ($(IS_OFA_KERNEL_MK_EXIST),yes)
            include $(OFA_KERNEL)/config.mk
        endif
        EXTRA_CFLAGS += $(BACKPORT_INCLUDES) 
        KBUILD_EXTRA_SYMBOLS += $(OFA_KERNEL)/Module.symvers 
endif

# MLNX_EN includes
MLNX_EN_SRPM_PKG='mellanox-mlnx-en-sources'
MLNX_EN_INCLUDES=$(shell rpm -qa $(MLNX_EN_SRPM_PKG) | grep $(MLNX_EN_SRPM_PKG) > /dev/null && rpm -ql $(MLNX_EN_SRPM_PKG)  | head -1)
ifneq (,$(MLNX_EN_INCLUDES))
OFA_INCLUDES += -I$(MLNX_EN_INCLUDES)/include -I$(MLNX_EN_INCLUDES)/include/rdma
endif

# add kenrel includes to OFA_INCLUDES
ifneq (,$(KSRC))
OFA_INCLUDES += -I$(KSRC)/include -I$(KSRC)/include/rdma 
endif

#$(warning *** OFA_INCLUDES=$(OFA_INCLUDES),)
