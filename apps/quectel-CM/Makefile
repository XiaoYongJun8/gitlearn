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

BASE_PATH      		:= $(shell cd $(shell pwd)/../../..; pwd)
ROOTFS_DIR     		:= $(BASE_PATH)/rootfs
ROOTFS_USR_SBIN_DIR := $(ROOTFS_DIR)/usr/sbin

LY_APP_TARGET  = quectel-CM

ifneq ($(CROSS_COMPILE),)
CROSS-COMPILE:=$(CROSS_COMPILE)
endif
#CROSS-COMPILE:=/workspace/buildroot/buildroot-qemu_mips_malta_defconfig/output/host/usr/bin/mips-buildroot-linux-uclibc-
#CROSS-COMPILE:=/workspace/buildroot/buildroot-qemu_arm_vexpress_defconfig/output/host/usr/bin/arm-buildroot-linux-uclibcgnueabi-
#CROSS-COMPILE:=/workspace/buildroot-git/qemu_mips64_malta/output/host/usr/bin/mips-gnu-linux-
ifeq ($(CC),cc)
CC:=$(CROSS-COMPILE)gcc
endif
LD:=$(CROSS-COMPILE)ld

QL_CM_SRC=QmiWwanCM.c GobiNetCM.c main.c MPQMUX.c QMIThread.c util.c qmap_bridge_mode.c mbim-cm.c device.c
QL_CM_SRC+=atc.c atchannel.c at_tok.c
#QL_CM_SRC+=qrtr.c rmnetctl.c
ifeq (1,1)
QL_CM_DHCP=udhcpc.c
else
LIBMNL=libmnl/ifutils.c libmnl/attr.c libmnl/callback.c libmnl/nlmsg.c libmnl/socket.c
DHCP=libmnl/dhcp/dhcpclient.c libmnl/dhcp/dhcpmsg.c libmnl/dhcp/packet.c
QL_CM_DHCP=udhcpc_netlink.c
QL_CM_DHCP+=${LIBMNL}
endif

CFLAGS += -Wall -g -Werror -O1 #-s
LDFLAGS += -lpthread -ldl -lrt

release: clean qmi-proxy mbim-proxy
	$(CC) ${CFLAGS} ${QL_CM_SRC} ${QL_CM_DHCP} -o $(LY_APP_TARGET) ${LDFLAGS}

debug: clean
	$(CC) ${CFLAGS} -g -DCM_DEBUG ${QL_CM_SRC} ${QL_CM_DHCP} -o $(LY_APP_TARGET) -lpthread -ldl -lrt

qmi-proxy:
	$(CC) ${CFLAGS} quectel-qmi-proxy.c -o quectel-qmi-proxy ${LDFLAGS} 

mbim-proxy:
	$(CC) ${CFLAGS} quectel-mbim-proxy.c -o quectel-mbim-proxy ${LDFLAGS} 

clean:
	rm -rf *.o libmnl/*.o $(LY_APP_TARGET) quectel-qmi-proxy quectel-mbim-proxy
	
install: 
	install -d $(ROOTFS_USR_SBIN_DIR)
	install $(LY_APP_TARGET) $(ROOTFS_USR_SBIN_DIR)/$(LY_APP_TARGET)

uninstall:
	-rm -f $(ROOTFS_USR_SBIN_DIR)/$(LY_APP_TARGET)