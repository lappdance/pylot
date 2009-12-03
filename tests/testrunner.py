import sys
import os
import re
import unittest

sys.path.append("..")
import pylot
import utils

def loadTests():
	path = os.path.abspath(os.path.dirname(sys.argv[0]))
	files = os.listdir(path)

	testPattern = lambda s : s.startswith("test_")
	files = filter(testPattern, files)
	
	filenameToModuleName = lambda f: os.path.splitext(f)[0]
	moduleNames = map(filenameToModuleName, files)
	
	modules = map(__import__, moduleNames)
	
	load = unittest.defaultTestLoader.loadTestsFromModule
	return unittest.TestSuite(map(load, modules))

if __name__ == "__main__":
	pylot.enterBenchMode()
	pylot.globals.PI_QuietMode = 1
	pylot.globals.PI_OnErrorReturn = 1

	suite = loadTests()
	
	stream = utils.BlackHole() if pylot.mpi_rank != 0 else sys.stderr
	unittest.TextTestRunner(stream=stream, verbosity=2).run(suite)

	pylot.exitBenchMode()
	
