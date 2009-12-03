import unittest
import sys
import utils

sys.path.append("..")
import pylot

fromProducer = None

def producer(n, widget):
	pylot.write(fromProducer, *[widget() for i in range(n)])

class TestReadingVarArgs(unittest.TestCase):
	def setUp(self):
		global fromProducer
	
		pylot.configure()
		producerproc = pylot.createProcess(producer, 3, int)

		fromProducer = pylot.createChannel(producerproc, None)

		self.rank = pylot.startAll()
	
	def tearDown(self):
		if self.rank == 0:
			pylot.stopMain(0)
	
	def testReadAllItemsAtOnce(self):
		if self.rank == 0:
			global fromProducer
			
			values = pylot.read(fromProducer, 3)
			
			self.assertFalse(pylot.channelHasData(fromProducer))
	
	def testReadItemsInTwoStepsAsLists(self):
		if self.rank == 0:
			global fromProducer
			
			pylot.read(fromProducer, 1)
			pylot.read(fromProducer, 2)
			
			self.assertFalse(pylot.channelHasData(fromProducer))
	
	def testReadItemsAsSingleItems(self):
		if self.rank == 0:
			global fromProducer
			
			for i in range(3):
				pylot.read(fromProducer)
			
			self.assertFalse(pylot.channelHasData(fromProducer))
	
	def testReadItemsAsListsAndSingles(self):
		if self.rank == 0:
			global fromProducer
			
			pylot.read(fromProducer, 2)
			pylot.read(fromProducer)
			
			self.assertFalse(pylot.channelHasData(fromProducer))
	
	def testReadReturnsLists(self):
		if self.rank == 0:
			global fromProducer
			
			single = pylot.read(fromProducer)
			multiple = pylot.read(fromProducer, 1)
			
			self.assertEquals(int, type(single))
			self.assertEquals(list, type(multiple))
			self.assertEquals(1, len(multiple))
			self.assertEquals(single, multiple[0])
			
			pylot.read(fromProducer)
		

if __name__ == "__main__":
	pylot.enterBenchMode()
	pylot.globals.PI_QuietMode = 1

	suite = unittest.TestLoader().loadTestsFromTestCase(TestReadingVarArgs)
	
	stream = utils.BlackHole() if pylot.mpi_rank != 0 else sys.stderr
	unittest.TextTestRunner(stream=stream, verbosity=2).run(suite)

	pylot.exitBenchMode()
	
