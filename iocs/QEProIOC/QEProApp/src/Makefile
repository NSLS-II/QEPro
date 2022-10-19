TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE

PROD_NAME = QEProApp
PROD_IOC += $(PROD_NAME)

# <name>.dbd will be created from <name>Include.dbd
DBD += $(PROD_NAME).dbd
$(PROD_NAME)_DBD += QEProSupport.dbd
$(PROD_NAME)_DBD += base.dbd asyn.dbd busySupport.dbd asSupport.dbd devIocStats.dbd

# <name>_registerRecordDeviceDriver.cpp will be created from <name>.dbd
$(PROD_NAME)_SRCS += $(PROD_NAME)_registerRecordDeviceDriver.cpp $(PROD_NAME)Main.cpp


$(PROD_NAME)_LIBS += seabreeze asyn QEPro busy autosave devIocStats $(EPICS_BASE_IOC_LIBS)

# If using a system version of libuvc (i.e. make install) use the below line instead.
$(PROD_NAME)_SYS_LIBS += usb

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
#
#
