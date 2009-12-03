import ctypes as _ctypes
_libmpi = _ctypes.CDLL("libmpi.so", _ctypes.RTLD_GLOBAL)

import pylot as _pylot
import sys as _sys
import traceback as _traceback

class _StackTrace:
	def __init__(self, functor):
		self.functor = functor
	
	def __call__(self, *args, **kwargs):
		stack = _traceback.extract_stack()
		filename, lineno, funcname, line = stack[-2]
		_pylot.cvar.PI_CallerFile = filename
		_pylot.cvar.PI_CallerLine = lineno
		return self.functor(*args, **kwargs)


mpi_rank = 0
mpi_worldsize = 0

MAIN = None
BROADCAST = _pylot.PI_BROADCAST
GATHER = _pylot.PI_GATHER
SELECT = _pylot.PI_SELECT
SAME = _pylot.PI_SAME
REVERSE = _pylot.PI_REVERSE

globals = _pylot.cvar

@_StackTrace
def enterBenchMode(argv=_sys.argv):
	global mpi_rank, mpi_worldsize
	
	mpi_rank, mpi_worldsize = _pylot.enterBenchMode(argv)
	
	
exitBenchMode = _StackTrace(_pylot.exitBenchMode)

@_StackTrace
def configure(argv=_sys.argv):
	global mpi_rank, mpi_worldsize
	
	mpi_rank, mpi_worldsize = _pylot.PI_Configure_(argv)

createProcess = _StackTrace(_pylot.PI_CreateProcess_)
createChannel = _StackTrace(_pylot.PI_CreateChannel_)
createBundle = _StackTrace(_pylot.PI_CreateBundle_)
copyChannels = _StackTrace(_pylot.PI_CopyChannels_)
getName = _StackTrace(_pylot.PI_GetName_)
setName = _StackTrace(_pylot.PI_SetName_)
startAll = _StackTrace(_pylot.PI_StartAll_)
stopMain = _StackTrace(_pylot.PI_StopMain_)
select = _StackTrace(_pylot.PI_Select_)
trySelect = _StackTrace(_pylot.PI_TrySelect_)
channelHasData = _StackTrace(_pylot.PI_ChannelHasData_)
getBundleChannel = _StackTrace(_pylot.PI_GetBundleChannel_)
getBundleSize = _StackTrace(_pylot.PI_GetBundleSize_)

broadcast = _StackTrace(_pylot.PI_Broadcast_)
gather = _StackTrace(_pylot.PI_Gather_)

startTime = _pylot.PI_StartTime
endTime = _pylot.PI_EndTime
log = _StackTrace(_pylot.PI_Log_)
isLogging = _pylot.PI_IsLogging
abort = _pylot.PI_Abort

write = _StackTrace(_pylot.PI_Write_)
read = _StackTrace(_pylot.PI_Read_)

