#!/bin/bash

export LD_LIBRARY_PATH=/epics/iocs/QEPro/lib/linux-x86_64:$LD_LIBRARY_PATH

cd iocs/QEProIOC/iocBoot/iocQEPro && ./st.cmd
