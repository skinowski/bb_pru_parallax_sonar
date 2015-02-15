#!/bin/bash
#
# Enable PRU DTS in CapeMgr
#
#
CAPE_MGR=`ls -d /sys/devices/bone_capemgr.* 2>/dev/null`
if [ -z "$CAPE_MGR" ]; then
	echo "error cape_mgr: $CAPE_MGR"
	exit -1
fi

# Enable PRU
echo "BB-BONE-PRU" > ${CAPE_MGR}/slots
