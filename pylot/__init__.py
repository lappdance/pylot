import ctypes
_libmpi = ctypes.cdll.LoadLibrary("libmpi.so")

import pylot

PI_MAIN = pylot.PI_MAIN
process = pylot.PI_CreateProcess_
channel = pylot.PI_CreateChannel_
PI_BROADCAST = pylot.PI_BROADCAST
PI_GATHER = pylot.PI_GATHER
PI_SELECT = pylot.PI_SELECT
bundle = pylot.PI_CreateBundle_
PI_SAME = pylot.PI_SAME
PI_REVERSE = pylot.PI_REVERSE
copyChannels = pylot.PI_CopyChannels_
setName = pylot.PI_SetName_
startAll = pylot.PI_StartAll_
getName = pylot.PI_GetName_
select = pylot.PI_Select_
channelHasData = pylot.PI_ChannelHasData_
trySelect = pylot.PI_TrySelect_
getBundleChannel = pylot.PI_GetBundleChannel_
getBundleSize = pylot.PI_GetBundleSize_
broadcast = pylot.PI_Broadcast_
gather = pylot.PI_Gather_
startTime = pylot.PI_StartTime
endTime = pylot.PI_EndTime
log = pylot.PI_Log_
isLogging = pylot.PI_IsLogging
abort = pylot.PI_Abort
stopMain = pylot.PI_StopMain_
configure = pylot.PI_Configure_

write = pylot.PI_Write_
read = pylot.PI_Read_
