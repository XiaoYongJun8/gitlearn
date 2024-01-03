# /********************************************************************************
#  *                        Copyright (C), 2022, LinYuan Power Inc.
#  ********************************************************************************
#  * File:           Makefile
#  * Description:    build ipc lib
#  * Author:         zhums
#  * E-Mail:         zhums@linygroup.com
#  * Version:        1.0
#  * Date:           2022-11-07
#  * IDE:            Visual Studio Code 1.70.0
#  * 
#  * History: 
#  * No.  Date            Author    Modification
#  * 1    2022-11-07      zhums     Created file
# ********************************************************************************/

BASE_PATH      := $(shell cd $(shell pwd)/../../..; pwd)
ROOTFS_DIR     := $(BASE_PATH)/rootfs
ROOTFS_LIB_DIR := $(ROOTFS_DIR)/lib
LY_PATH        := $(BASE_PATH)/liny
LYLIB_PATH     := $(LY_PATH)/libs
define TAB
	
endef

SRCS = $(wildcard ./*.c)  
OBJS = $(patsubst %.c, %.o, $(SRCS)) 
LY_LIB_TARGET=libipc.so

#--------------------------------------------------------
# Compile options
#--------------------------------------------------------
CFLAGS     += -Wall -g -fPIC
LY_CFLAGS  := -I. \
              -I$(LYLIB_PATH)/log

LDFLAGS    += -Wall -g -shared
LY_LDFLAGS := -L$(LYLIB_PATH)/log       \
              -llog

#--------------------------------------------------------
# Make rules
#--------------------------------------------------------
.PHONY : all clean distclean install uninstall

all: $(LY_LIB_TARGET)
	
$(LY_LIB_TARGET) : $(OBJS)
	$(CC) $(LDFLAGS) $(TAB) $^ -o $@ 

%.o: %.c
	$(CC) $(CFLAGS) $(LY_CFLAGS) $(TAB) -c $< -o $@

clean:
	-rm -f *.o 
distclean: 
	-rm -f *.o *.so $(LY_LIB_TARGET)
	
install: all
	install -d $(ROOTFS_LIB_DIR)
	install $(LY_LIB_TARGET) $(ROOTFS_LIB_DIR)/$(LY_LIB_TARGET)

uninstall:
	-rm -f $(ROOTFS_LIB_DIR)/$(LY_LIB_TARGET)

