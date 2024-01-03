# /********************************************************************************
#  *                        Copyright (C), 2022, LinYuan Power Inc.
#  ********************************************************************************
#  * File:           Makefile
#  * Description:    used to build suxin libs
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

LOG_DIR      = log
JSON_DIR     = json
COMMON_DIR   = common
IPC_DIR      = ipc
HTTP_DIR     = http
MODBUS_DIR   = modbus
DATABASE_DIR = database

DIRS :=$(LOG_DIR)
DIRS +=$(JSON_DIR)
DIRS +=$(COMMON_DIR)
DIRS +=$(IPC_DIR)
DIRS +=$(HTTP_DIR)
DIRS +=$(MODBUS_DIR)
DIRS +=$(DATABASE_DIR)

.PHONY : all clean distclean install uninstall 
.PHONY : $(LOG_DIR) $(LOG_DIR)_clean 
.PHONY : $(JSON_DIR) $(JSON_DIR)_clean 
.PHONY : $(COMMON_DIR) $(COMMON_DIR)_clean 
.PHONY : $(IPC_DIR) $(IPC_DIR)_clean 
.PHONY : $(HTTP_DIR) $(HTTP_DIR)_clean 
.PHONY : $(MODBUS_DIR) $(MODBUS_DIR)_clean
.PHONY : $(DATABASE_DIR) $(DATABASE_DIR)_clean

all:
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d}; done

clean:
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} clean; done
	rm -f ./*/*.o
distclean:
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} distclean; done
	rm -f ./*/*.o

$(LOG_DIR):
	$(MAKE) -C $(LOG_DIR)
$(LOG_DIR)_clean:
	$(MAKE) -C $(LOG_DIR) clean
	
$(JSON_DIR):
	$(MAKE) -C $(JSON_DIR)
$(JSON_DIR)_clean:
	$(MAKE) -C $(JSON_DIR) clean

$(COMMON_DIR):
	$(MAKE) -C $(COMMON_DIR)
$(COMMON_DIR)_clean:
	$(MAKE) -C $(COMMON_DIR) clean

$(HTTP_DIR):
	$(MAKE) -C $(HTTP_DIR)
$(HTTP_DIR)_clean:
	$(MAKE) -C $(HTTP_DIR) clean

$(IPC_DIR):
	$(MAKE) -C $(IPC_DIR)
$(IPC_DIR)_clean:
	$(MAKE) -C $(IPC_DIR) clean

$(MODBUS_DIR):
	$(MAKE) -C $(MODBUS_DIR)
$(MODBUS_DIR)_clean:
	$(MAKE) -C $(MODBUS_DIR) clean

$(DATABASE_DIR):
	$(MAKE) -C $(DATABASE_DIR)
$(DATABASE_DIR)_clean:
	$(MAKE) -C $(DATABASE_DIR) clean

install:
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} install; done

uninstall:
	set -e; for d in ${DIRS}; do $(MAKE) -C $${d} uninstall; done	
