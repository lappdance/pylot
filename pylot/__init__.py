import ctypes
_libmpi = ctypes.CDLL("libmpi.so", ctypes.RTLD_GLOBAL)

import pylot
import sys

mpi_rank = 0
mpi_worldsize = 0

MAIN = None
BROADCAST = pylot.PI_BROADCAST
GATHER = pylot.PI_GATHER
SELECT = pylot.PI_SELECT
SAME = pylot.PI_SAME
REVERSE = pylot.PI_REVERSE

def enterBenchMode():
	global mpi_rank, mpi_worldsize
	
	mpi_rank, mpi_worldsize = pylot.enterBenchMode(sys.argv)
	
	
exitBenchMode = pylot.exitBenchMode

def configure():
	global mpi_rank, mpi_worldsize
	
	mpi_rank, mpi_worldsize = pylot.PI_Configure_(sys.argv)

createProcess = pylot.PI_CreateProcess_
createChannel = pylot.PI_CreateChannel_
createBundle = pylot.PI_CreateBundle_
copyChannels = pylot.PI_CopyChannels_
getName = pylot.PI_GetName_
setName = pylot.PI_SetName_
startAll = pylot.PI_StartAll_
stopMain = pylot.PI_StopMain_
select = pylot.PI_Select_
trySelect = pylot.PI_TrySelect_
channelHasData = pylot.PI_ChannelHasData_
getBundleChannel = pylot.PI_GetBundleChannel_
getBundleSize = pylot.PI_GetBundleSize_

broadcast = pylot.PI_Broadcast_

def gather(bundle, n=1):
	if n < 1:
		raise ValueError("you must gather at least one element")
	
	if n == 1:
		return pylot.PI_GatherItem(bundle);
	else:
		return [gather(bundle) for i in range(n)]

startTime = pylot.PI_StartTime
endTime = pylot.PI_EndTime
log = pylot.PI_Log_
isLogging = pylot.PI_IsLogging
abort = pylot.PI_Abort

write = pylot.PI_Write_
read = pylot.PI_Read_

