# Driver Name
DRIVER_NAME = pfmr_ktest

# Test C Files
CFILES = pfmr_test.c pfmr_main.c

# installed KVL SRC PATH
KVL_SRC_DIR = /usr/src/mlxkvl

# TARGET MODULE TO MAKE
TARGET = $(DRIVER_NAME).ko

# roles to make
all : Module.symvers modules

Module.symvers: $(KVL_SRC_DIR)/Module.symvers
	cp $< $@
# include KVL config to query OFA_INCLUDES and OFA_KERNEL src path
include $(KVL_SRC_DIR)/kvl_config.mk

# add extra_cflags needed by this module
EXTRA_CFLAGS += -I$(KVL_SRC_DIR)/include -I/usr/src/ofa_kernel-1.5.5/include 
#-I$(OFA_INCLUDES)

# include kvl_generic_makefile to prepare build enviorment (this is a must) 
# KSRC , KERNEL_VER , arch dependent extra_clags , backports , etc ...
include $(KVL_SRC_DIR)/kvl_generic_makefile.mk

modules:
	$(MAKE) -C $(KSRC) SUBDIRS=$(shell pwd) NOSTDINC_FLAGS="$(EXTRA_CFLAGS)" modules
clean:
	rm -rf $(TARGET) $(TARGET:.ko=.o) $(TARGET:.ko=.mod.c) $(TARGET:.ko=.ko.unsigned) $(TARGET:.ko=.mod.o) $(CFILES:.c=.o) \
	$(MANFILE).gz .*cmd .tmp_versions Module.markers Module.symvers modules.order

# for install/uninstall roles
ifneq (,$(MLXTST_SRCROOT))
include $(MLXTST_SRCROOT)/scripts/mlx_test_inc.mk
endif
