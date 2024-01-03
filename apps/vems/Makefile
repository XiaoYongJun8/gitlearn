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
LY_PATH        := $(BASE_PATH)/liny
LYLIB_PATH     := $(LY_PATH)/libs
define TAB
	
endef

SRCS = $(wildcard ./*.c)  
OBJS = $(patsubst %.c, %.o, $(SRCS)) 
LY_APP_TARGET=vems
CFG_APP      =config.ini

#--------------------------------------------------------
# Compile options
#--------------------------------------------------------
CFLAGS     += -Wall -g
LY_CFLAGS  := -I. \
              -I$(LYLIB_PATH)/log       \
              -I$(LYLIB_PATH)/common	\
              -I$(LYLIB_PATH)/modbus	\
			  -I$(LYLIB_PATH)/database	\
			  -I$(LYLIB_PATH)/dlt645	\
              -I$(LYLIB_PATH)/ipc       

LDFLAGS    += -Wall -g -lpthread -ldl
LY_LDFLAGS := -L$(LYLIB_PATH)/log       \
              -L$(LYLIB_PATH)/common    \
              -L$(LYLIB_PATH)/modbus    \
			  -L$(LYLIB_PATH)/database  \
			  -L$(LYLIB_PATH)/dlt645    \
              -L$(LYLIB_PATH)/ipc       \
              -llog -lcommon -lmodbus -ldatabase -ldlt645 -lipc 

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
	install -d $(ROOTFS_APPS_DIR)
	install $(LY_APP_TARGET) $(ROOTFS_APPS_DIR)/$(LY_APP_TARGET)
	install ../$(CFG_APP)       $(ROOTFS_APPS_DIR)/$(CFG_APP)

uninstall:
	-rm -f $(ROOTFS_APPS_DIR)/$(LY_APP_TARGET)
