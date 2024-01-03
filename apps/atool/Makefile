# /********************************************************************************
#  *                        Copyright (C), 2023, LinYuan Power Inc.
#  ********************************************************************************
#  * File:           Makefile
#  * Description:    build atool app
#  * Author:         zhums
#  * E-Mail:         zhums@linygroup.com
#  * Version:        1.0
#  * Date:           2023-11-26
#  * IDE:            Visual Studio Code 1.81.1
# ********************************************************************************/

BASE_PATH      := $(shell cd $(shell pwd)/../../..; pwd)
ROOTFS_DIR     := $(BASE_PATH)/rootfs
ROOTFS_HOME_DIR:= $(ROOTFS_DIR)/home/root
ROOTFS_LIB_DIR := $(ROOTFS_DIR)/lib
ROOTFS_APPS_DIR:= $(ROOTFS_HOME_DIR)/apps
ROOTFS_LBIN_DIR:= $(ROOTFS_DIR)/usr/local/sbin/
LY_PATH        := $(BASE_PATH)/liny
LYLIB_PATH     := $(LY_PATH)/libs
define TAB
	
endef

SRCS = $(wildcard ./*.c)  
OBJS = $(patsubst %.c, %.o, $(SRCS)) 
LY_APP_TARGET=atool

#--------------------------------------------------------
# Compile options
#--------------------------------------------------------
CFLAGS     += -Wall -g
LY_CFLAGS  := -I. \
              -I$(LYLIB_PATH)/ipc       

LDFLAGS    += -Wall -g 
LY_LDFLAGS := -L$(LYLIB_PATH)/ipc       \
              -lipc 

#--------------------------------------------------------
# Make rules
#--------------------------------------------------------
.PHONY : all clean distclean install uninstall

all: $(LY_APP_TARGET)
	
$(LY_APP_TARGET) : $(OBJS)
	$(CC) $(LDFLAGS) $(LY_LDFLAGS) $(TAB) $^ -o $@ 

%.o: %.c
	$(CC) $(CFLAGS) $(LY_CFLAGS) $(TAB) -c $< -o $@

clean:
	-rm -f *.o 
distclean: 
	-rm -f *.o $(LY_APP_TARGET)
	
install: all
	install -d $(ROOTFS_LBIN_DIR)
	install $(LY_APP_TARGET) $(ROOTFS_LBIN_DIR)/$(LY_APP_TARGET)

uninstall:
	-rm -f $(ROOTFS_LBIN_DIR)/$(LY_APP_TARGET)
