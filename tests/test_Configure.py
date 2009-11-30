import unittest
import sys

sys.path.append("..")
import pylot

def nothing(rank, void):
	pass

class TestBenchMode(unittest.TestCase):
	
	def testOne(self):
		print "1"
	
	def testTwo(self):
		print "2"

def setUp(self):
	print "up"
	pylot.configure()
	
	pylot.createProcess(nothing, 0, None)
	
	pylot.startAll()

def tearDown(self):
	print "down"
	pylot.stopMain(0)

if __name__ == "__main__":
	pylot.enterBenchMode()
	
	setUp(None)
	
	unittest.main()
	
#	tearDown(None)
	print "down"
	pylot.stopMain(0)

