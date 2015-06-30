all:
.PHONY: user modules modules_install

# To build modules outside of the kernel tree, we run "make"
# in the kernel source tree; the Makefile these then includes this
# Makefile once again.
# This conditional selects whether we are being included from the
# kernel Makefile or not.

# Driver Name
DRIVER_NAME = atomic_test

# Test C Files
CFILES = basic_module.c 

# installed KVL SRC PATH
KVL_SRC_DIR = /usr/src/mlxkvl

# TARGET MODULE TO MAKE
TARGET = $(DRIVER_NAME).ko

# roles to make
all : user modules

# KVL symbol table
KBUILD_EXTRA_SYMBOLS += $(KVL_SRC_DIR)/Module.symvers
#$(shell cp $(KVL_SRC_DIR)/Module.symvers Module.symvers)

# include kvl_generic_makefile to prepare build enviorment (this is a must)
# KSRC , KERNEL_VER , arch dependent extra_clags , backports , etc ...
include $(KVL_SRC_DIR)/kvl_generic_makefile.mk

# include KVL config to query OFA_INCLUDES and OFA_KERNEL src path & OFA Symbols
include $(KVL_SRC_DIR)/kvl_config.mk

# add extra_cflags needed by this module
EXTRA_CFLAGS += -I$(KVL_SRC_DIR)/include $(OFA_INCLUDES)

user:
	make -f Makefile.user

modules:
	$(MAKE) -C $(KSRC) SUBDIRS=$(shell pwd) NOSTDINC_FLAGS="$(EXTRA_CFLAGS)" modules
clean:
	rm -rf $(TARGET) $(TARGET:.ko=.o) $(TARGET:.ko=.ko.unsigned) $(TARGET:.ko=.mod.c) $(TARGET:.ko=.mod.o) $(CFILES:.c=.o) \
	$(MANFILE).gz .*cmd .tmp_versions Module.markers Module.symvers modules.order 
	make -f Makefile.user clean

# for install/uninstall roles
ifneq (,$(MLXTST_SRCROOT))
include $(MLXTST_SRCROOT)/scripts/mlx_test_inc.mk
endif
