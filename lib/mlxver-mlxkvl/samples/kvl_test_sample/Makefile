# Driver Name
DRIVER_NAME = mlxkvl_sample

# Test C Files
CFILES = main.c
# installed KVL SRC PATH
KVL_SRC_DIR = /usr/src/mlxkvl

# TARGET MODULE TO MAKE
TARGET = $(DRIVER_NAME).ko

# roles to make
all :  modules

user_run : user_main.c 
	$(CC) $< -o $@ 

# KVL symbol table
KBUILD_EXTRA_SYMBOLS += $(KVL_SRC_DIR)/Module.symvers  

# include KVL config to query OFA_INCLUDES and OFA_KERNEL src path
include $(KVL_SRC_DIR)/kvl_config.mk  


# add extra_cflags needed by this module
EXTRA_CFLAGS += -I$(KVL_SRC_DIR)/include -I$(OFA_INCLUDES)  


# include kvl_generic_makefile to prepare build enviorment (this is a must) 
# KSRC , KERNEL_VER , arch dependent extra_clags , backports , etc ...
include $(KVL_SRC_DIR)/kvl_generic_makefile.mk 

modules:
	$(MAKE) -C $(KSRC) SUBDIRS=$(shell pwd) modules
clean:
	rm -rf $(TARGET) $(TARGET:.ko=.o) $(TARGET:.ko=.ko.unsigned) $(TARGET:.ko=.mod.c) $(TARGET:.ko=.mod.o) $(CFILES:.c=.o) \
	$(MANFILE).gz .*cmd .tmp_versions Module.markers Module.symvers modules.order user_run
