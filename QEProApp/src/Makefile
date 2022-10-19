TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS AFTER THIS LINE
#
#  #======== DETECTOR LIBRARY ==============

USR_CXXFLAGS_Linux += -D_LINUX -D_x86 -std=c++98
USR_CFLAGS_Linux += -std=gnu99

# The following is needed on win32-x86-debug because the Prosilica library 
# wants to use LIBCMT, and that conflicts with LIBCMTD

LIBRARY_IOC += QEPro

INC += QEPro.hpp

LIB_SRCS += QEPro.cpp

LIB_LIBS += seabreeze

DBD += QEProSupport.dbd


# #=============================

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD RULES AFTER THIS LINE
