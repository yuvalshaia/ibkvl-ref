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

# core driver files
DRIVER_NAME = mlxkvl

CFILES = mlxkvl-main.c libs/vl_os.c libs/vl_rand.c libs/vl_trace.c
HFILES = kvl_common.h
INCLUDE_DIR = include
USR_INCLUDE_DIR = user/include
KVLSRC?=$(shell pwd)

ifeq (,$(BUILD_KERNEL))
BUILD_KERNEL=$(shell uname -r)
endif

#$(warning *** pwd:$(KVLSRC))
include $(KVLSRC)/kvl_generic_makefile.mk

include $(KVLSRC)/kvl_config.mk

#ifneq ($(OFA_INCLUDES),)
	CFILES += libs/vl_gen2k_str.c
	$(DRIVER_NAME)-objs += libs/vl_gen2k_str.o
#endif

EXTRA_CFLAGS += -I$(KVLSRC)/include $(OFA_INCLUDES) 

###########################################################################
# 2.6.x Specific rules

K_VERSION:=$(shell uname -r | cut -c1-3 | sed 's/2\.[56]/2\.6/')

# Makefile for 2.6.x kernel
TARGET = $(DRIVER_NAME).ko
# man page
MANSECTION = 7
MANFILE = $(TARGET:.ko=.$(MANSECTION))
        
# MAKE 
default:
ifeq ($(KOBJ),$(KSRC))
	$(MAKE) -C $(KSRC) SUBDIRS=$(KVLSRC) KVLSRC=$(KVLSRC) NOSTDINC_FLAGS="$(EXTRA_CFLAGS)" modules
else
	$(MAKE) -C $(KSRC) O=$(KOBJ) SUBDIRS=$(KVLSRC) KVLSRC=$(KVLSRC) modules
endif

ifeq (,$(MANDIR))
  # find the best place to install the man page
  MANPATH := $(shell (manpath 2>/dev/null || echo $MANPATH) | sed 's/:/ /g')
  ifneq (,$(MANPATH))
    # test based on inclusion in MANPATH
    test_dir = $(findstring $(dir), $(MANPATH))
  else
    # no MANPATH, test based on directory existence
    test_dir = $(shell [ -e $(dir) ] && echo $(dir))
  endif
  # our preferred install path
  # should /usr/local/man be in here ?
  MANDIR := /usr/share/man /usr/man
  MANDIR := $(foreach dir, $(MANDIR), $(test_dir))
  MANDIR := $(firstword $(MANDIR))
endif
ifeq (,$(MANDIR))
  # fallback to /usr/man
  MANDIR := /usr/man
endif

# depmod version for rpm builds
DEPVER := $(shell /sbin/depmod -V 2>/dev/null | \
          awk 'BEGIN {FS="."} NR==1 {print $$2}')

###########################################################################
# Build rules
# prepare  man file
$(MANFILE).gz: ../$(MANFILE)
	gzip -c $< > $@

# install
install: default $(MANFILE).gz
	# remove all old versions of the driver
	find $(INSTALL_MOD_PATH)/lib/modules/$(KVER) -name $(TARGET) -exec rm -f {} \; || true
	find $(INSTALL_MOD_PATH)/lib/modules/$(KVER) -name $(TARGET).gz -exec rm -f {} \; || true
	install -D -m 644 $(TARGET) $(INSTALL_MOD_PATH)$(INSTDIR)/$(TARGET)
ifeq (,$(INSTALL_MOD_PATH))
	/sbin/depmod -a || true
else
  ifeq ($(DEPVER),1 )
	/sbin/depmod -r $(INSTALL_MOD_PATH) -a || true
  else
	/sbin/depmod -b $(INSTALL_MOD_PATH) -a -n $(KVERSION) > /dev/null || true
  endif
endif
	install -D -m 644 $(MANFILE).gz $(INSTALL_MOD_PATH)$(MANDIR)/man$(MANSECTION)/$(MANFILE).gz
	man -c -P'cat > /dev/null' $(MANFILE:.$(MANSECTION)=) || true
	mkdir -p $(INSTALL_MOD_PATH)/usr/src/mlxkvl/include
	mkdir -p $(INSTALL_MOD_PATH)/usr/bin
	mkdir -p $(INSTALL_MOD_PATH)/usr/include/mlxkvl
	install -D -m 644 $(INCLUDE_DIR)/* $(INSTALL_MOD_PATH)/usr/src/mlxkvl/include
	install -D -m 644 $(USR_INCLUDE_DIR)/* $(INSTALL_MOD_PATH)/usr/include/mlxkvl
	install -D -m 644 Module*.symvers $(INSTALL_MOD_PATH)/usr/src/mlxkvl/
	install -D -m 644 kvl_config.mk  $(INSTALL_MOD_PATH)/usr/src/mlxkvl/
	install -D -m 644 kvl_generic_makefile.mk $(INSTALL_MOD_PATH)/usr/src/mlxkvl/
	install -D -m 777 scripts/mlxkvl $(INSTALL_MOD_PATH)/usr/bin/


uninstall:
	if [ -e $(INSTDIR)/$(TARGET) ] ; then \
	    rm -f $(INSTDIR)/$(TARGET) ; \
	fi
	/sbin/depmod -a
	if [ -e $(MANDIR)/man$(MANSECTION)/$(MANFILE).gz ] ; then \
		rm -f $(MANDIR)/man$(MANSECTION)/$(MANFILE).gz ; \
	fi
	if [ -e $(INSTALL_MOD_PATH)/usr/src/mlxkvl ] ; then \
		rm -rf $(INSTALL_MOD_PATH)/usr/src/mlxkvl/ ; \
	fi
	if [ -e $(INSTALL_MOD_PATH)/usr/include/mlxkvl ] ; then \
		rm -rf $(INSTALL_MOD_PATH)/usr/include/mlxkvl ; \
	fi

.PHONY: clean install

clean:
	rm -rf $(TARGET) $(TARGET:.ko=.o) $(TARGET:.ko=.mod.c) $(TARGET:.ko=.mod.o) $(CFILES:.c=.o) \
	$(MANFILE).gz .*cmd .tmp_versions Module.markers Module.symvers modules.order lib/.*cmd lib/.tmp_versions
