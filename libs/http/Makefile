# /********************************************************************************
#  *                        Copyright (C), 2022, LinYuan Power Inc.
#  ********************************************************************************
#  * File:           Makefile
#  * Description:    build http lib
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
KERNEL_INC_PATH:= $(BASE_PATH)/kheaders/include
LY_PATH        := $(BASE_PATH)/liny
LYLIB_PATH     := $(LY_PATH)/libs
define TAB
	
endef

ALLC = $(wildcard *.c)
SRCS = $(filter-out test.c, $(ALLC)) 
OBJS = $(patsubst %.c, %.o, $(SRCS)) 
LY_LIB_TARGET=libhttp.so

#--------------------------------------------------------
# Compile options
#--------------------------------------------------------
CFLAGS     += -Wall -g -fPIC
LY_CFLAGS  := -I. \
              -I$(LYLIB_PATH)/log       \
              -I$(LYLIB_PATH)/json
TESTLDFLAGS:= -Wall -g $(LDFLAGS) -I$(LYLIB_PATH)/log -L. -lhttp -lcurl
LDFLAGS    += -Wall -g -shared
LY_LDFLAGS := -L$(LYLIB_PATH)/log  \
              -L$(LYLIB_PATH)/json \
              -llog -ljson

#--------------------------------------------------------
# Make rules
#--------------------------------------------------------
.PHONY : all test htest clean distclean install uninstall

all: $(LY_LIB_TARGET)
	
$(LY_LIB_TARGET) : $(OBJS)
	$(CC) $(LDFLAGS) $(LY_LDFLAGS) $(TAB) $^ -o $@ 

%.o: %.c
	$(CC) $(CFLAGS) $(LY_CFLAGS) $(TAB) -c $< -o $@

test: test.c
	$(CC) $(TESTLDFLAGS) $(LY_LDFLAGS) $(TAB) $^ -o $@ 
	mv test testhttp
htest:
	gcc -Wall -g http.c buffer.c test.c -I/usr/include/curl -L/usr/lib/x86_64-linux-gnu/ -lcurl -o test
# -m64 -L/usr/lib/x86_64-linux-gnu
clean:
	-rm -f *.o 
distclean: 
	-rm -f *.o *.so test $(LY_LIB_TARGET) 
	
install: all
	install -d $(ROOTFS_LIB_DIR)
	install $(LY_LIB_TARGET) $(ROOTFS_LIB_DIR)/$(LY_LIB_TARGET)

uninstall:
	-rm -f $(ROOTFS_LIB_DIR)/$(LY_LIB_TARGET)

