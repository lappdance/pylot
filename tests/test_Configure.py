import unittest
import sys
import utils

sys.path.append("..")
import pylot

class TestRepeatedConfigures(unittest.TestCase):
	def setUp(self):
		pylot.configure()
		
	def testConfigure(self):
		self.assertTrue(True)
	
	def testConfigureAgain(self):
		self.assertTrue(True)

if __name__ == "__main__":
	pylot.enterBenchMode()
	pylot.globals.PI_QuietMode = 1
	
	suite = unittest.TestLoader().loadTestsFromTestCase(TestRepeatedConfigures)
	stream = utils.BlackHole() if pylot.mpi_rank != 0 else sys.stderr
	unittest.TextTestRunner(stream=stream, verbosity=2).run(suite)

	pylot.exitBenchMode()

