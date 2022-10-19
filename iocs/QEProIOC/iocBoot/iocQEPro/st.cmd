#!../../bin/linux-x86_64/QEProApp

epicsEnvSet("LOCATION", "LOB1 LAB9")
epicsEnvSet("Engineer", "J. Wlodek")
errlogInit(20000)
< envPaths

dbLoadDatabase("$(TOP)/dbd/QEProApp.dbd")

QEProApp_registerRecordDeviceDriver(pdbbase) 

epicsEnvSet(EPICS_CA_MAX_ARRAY_BYTES,64000000)
epicsEnvSet("EPICS_CA_AUTO_ADDR_LIST",         "NO")
epicsEnvSet("EPICS_CA_ADDR_LIST",              "127.0.0.255")
epicsEnvSet("EPICS_CAS_INTF_ADDR_LIST",              "127.0.0.1")

epicsEnvSet("PREFIX", "XF:28ID2-ES{QEPro:Spec-1}")
epicsEnvSet("PORT",   "QEPro")


QEProConfig("$(PORT)", 0)

#asynSetTraceMask("$(PORT)", -1, 0x0)
#asynSetTraceMask("$(PORT)", -1, 0x1)

# Enables both log and error messages
asynSetTraceMask("$(PORT)", -1, 0x9)
#asynSetTraceMask("$(PORT)", -1, 0xF)
#asynSetTraceMask("$(PORT)", -1, 0x11)
#asynSetTraceMask("$(PORT)", -1, 0xFF)
#asynSetTraceIOMask("$(PORT)", -1, 0x0)
#asynSetTraceIOMask("$(PORT)", -1, 0x2)

dbLoadRecords("$(QEPRO)/db/QEPro.template", "PREFIX=$(PREFIX), PORT=$(PORT), ADDR=0, TIMEOUT=1")
dbLoadRecords("$(QEPRO)/db/QEProCollect.template", "PREFIX=$(PREFIX), PORT=$(PORT), ADDR=0, TIMEOUT=1")
dbLoadRecords("$(QEPRO)/db/QEProSpectra.template", "PREFIX=$(PREFIX), PORT=$(PORT), ADDR=0, TIMEOUT=1")
dbLoadRecords("$(EPICS_BASE)/db/asynRecord.db", "PORT=$(PORT), P=$(PREFIX), R=:ASYNIO, ADDR=0, OMAX=0, IMAX=0")
dbLoadRecords("$(EPICS_BASE)/db/iocAdminSoft.db", "IOC=$(PREFIX)")


## autosave/restore machinery
#save_restoreSet_Debug(0)
#save_restoreSet_IncompleteSetsOk(1)
#save_restoreSet_DatedBackupFiles(1)
 
#set_savefile_path("as","/sav")
#set_requestfile_path("as","/req")
 
#set_pass0_restoreFile("info_positions.sav")
#set_pass0_restoreFile("info_settings.sav")
#set_pass1_restoreFile("info_settings.sav")
 
iocInit()
 
## more autosave/restore machinery
#cd autosave/req
#makeAutosaveFiles()
#cd ../..
#create_monitor_set("info_positions.req", 5 , "")
#create_monitor_set("info_settings.req", 15 , "")

