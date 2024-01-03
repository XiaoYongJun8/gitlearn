# /********************************************************************************
#  *                        Copyright (C), 2023, LinYuan Power Inc.
#  ********************************************************************************
#  * File:           Makefile
#  * Description:    build vems app
#  * Author:         zhums
#  * E-Mail:         zhums@linygroup.com
#  * Version:        1.0
#  * Date:           2023-10-07
#  * IDE:            Visual Studio Code 1.81.1
# ********************************************************************************/

BASE_PATH      := $(shell cd $(shell pwd)/../../..; pwd)
ROOTFS_DIR     := $(BASE_PATH)/rootfs
ROOTFS_HOME_DIR:= $(ROOTFS_DIR)/home/root
ROOTFS_LIB_DIR := $(ROOTFS_DIR)/lib
ROOTFS_APPS_DIR:= $(ROOTFS_HOME_DIR)/apps
ROOTFS_TEST_DIR:= $(ROOTFS_HOME_DIR)/test
SX_PATH        := $(BASE_PATH)/suxin
SXLIB_PATH     := $(SX_PATH)/libs
define TAB
	
endef

SRCS = $(wildcard ./*.c)  
OBJS = $(patsubst %.c, %.o, $(SRCS)) 
SX_APP_TARGET=hw_test

#--------------------------------------------------------
# Compile options
#--------------------------------------------------------
CFLAGS     += -Wall -g
SX_CFLAGS  := -I. \
              -I$(SXLIB_PATH)/log       \
              -I$(SXLIB_PATH)/common
LDFLAGS    += -Wall -g -lpthread
SX_LDFLAGS := -L$(SXLIB_PATH)/log       \
              -L$(SXLIB_PATH)/common    \
              -llog -lcommon

#--------------------------------------------------------
# Make rules
#--------------------------------------------------------
.PHONY : all clean distclean install uninstall

all: $(SX_APP_TARGET)
	
$(SX_APP_TARGET) : $(OBJS)
	$(CC) $(LDFLAGS) $(TAB) $^ -o $@ 

%.o: %.c
	$(CC) $(CFLAGS) $(SX_CFLAGS) $(TAB) -c $< -o $@

clean:
	-rm -f *.o 
distclean: 
	-rm -f *.o $(SX_APP_TARGET)
	
install: all
	install -d $(ROOTFS_APPS_DIR)
	install $(SX_APP_TARGET) $(ROOTFS_TEST_DIR)/$(SX_APP_TARGET)

uninstall:
	-rm -f $(ROOTFS_TEST_DIR)/$(SX_APP_TARGET)
