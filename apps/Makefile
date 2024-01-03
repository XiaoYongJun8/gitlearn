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

ATOOL_DIR     = atool
QUECTELCM_DIR = quectel-CM
NETCONN_DIR   = netconn
VEMS_DIR      = vems

DIRS =  $(ATOOL_DIR)
DIRS += $(QUECTELCM_DIR)
DIRS += $(NETCONN_DIR)
DIRS += $(VEMS_DIR)

.PHONY : all clean distclean install uninstall 
.PHONY : $(ATOOL_DIR) $(ATOOL_DIR)_clean 
.PHONY : $(QUECTELCM_DIR) $(QUECTELCM_DIR)_clean 
.PHONY : $(NETCONN_DIR)   $(NETCONN_DIR)_clean 
.PHONY : $(VEMS_DIR)   $(VEMS_DIR)_clean 

all:
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d}; done

clean:
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} clean; done
distclean:
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} distclean; done

$(ATOOL_DIR):
	$(MAKE) -C $(ATOOL_DIR)
$(ATOOL_DIR)_clean:
	$(MAKE) -C $(ATOOL_DIR) clean

$(QUECTELCM_DIR):
	$(MAKE) -C $(QUECTELCM_DIR)
$(QUECTELCM_DIR)_clean:
	$(MAKE) -C $(QUECTELCM_DIR) clean

$(NETCONN_DIR):
	$(MAKE) -C $(NETCONN_DIR)
$(NETCONN_DIR)_clean:
	$(MAKE) -C $(NETCONN_DIR) clean

$(VEMS_DIR):
	$(MAKE) -C $(VEMS_DIR)
$(VEMS_DIR)_clean:
	$(MAKE) -C $(VEMS_DIR) clean
	
install:
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} install; done

uninstall:
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} uninstall; done
