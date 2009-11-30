import unittest
import sys

sys.path.append("..")
import pylot

class BlackHole:
	def write(self, *args):
		pass
	def flush(self):
		pass

toEchoer = None
fromEchoer = None

def echoer(i, j):
	global toEchoer, fromEchoer
	
	n = pylot.read(toEchoer)
	pylot.write(fromEchoer, *pylot.read(toEchoer, n))

class TestWritingVarArgs(unittest.TestCase):
	def setUp(self):
		global toEchoer, fromEchoer
	
		pylot.configure()
		echoproc = pylot.createProcess(echoer, 0, None)

		toEchoer = pylot.createChannel(None, echoproc)
		fromEchoer = pylot.createChannel(echoproc, None)

		self.rank = pylot.startAll()
	
	def tearDown(self):
		if self.rank == 0:
			pylot.stopMain(0)
	
	def testOneItemSeperately(self):
		if self.rank == 0:
			global toEchoer, fromEchoer
			
			pylot.write(toEchoer, 1)
			pylot.write(toEchoer, 6.7)
			values = pylot.read(fromEchoer, 1)
	
			self.assertEqual(1, len(values))
			self.assertEqual(6.7, values[0])
	
	def testOneItemInOneSend(self):
		if self.rank == 0:
			global toEchoer, fromEchoer
			
			pylot.write(toEchoer, 1, 44)
			values = pylot.read(fromEchoer, 1)
			
			self.assertEqual(1, len(values))
			self.assertEqual(44, values[0])
	
	def testThreeItems(self):
		if self.rank == 0:
			global toEchoer, fromEchoer
			
			pylot.write(toEchoer, 3, "nothing", None, -5)
			values = pylot.read(fromEchoer, 3)
			
			self.assertEqual(3, len(values))
			self.assertEqual("nothing", values[0])
			self.assertEqual(None, values[1])
			self.assertEqual(-5, values[2])
			
	def testListIsStillOneItem(self):
		if self.rank == 0:
			global toEchoer, fromEchoer
			
			l = [1, 2, 3]
			pylot.write(toEchoer, 1, l)
			values = pylot.read(fromEchoer, 1)
			
			self.assertEqual(1, len(values))
			self.assertNotEqual(l, values)
			self.assertEqual(l, values[0])

	def testExplodedListIsMoreThanOneItem(self):
		if self.rank == 0:
			global toEchoer, fromEchoer
			
			l = [3.14, 6.67, 2.13]
			pylot.write(toEchoer, 3, *l)
			values = pylot.read(fromEchoer, 3)
			
			self.assertEquals(3, len(values))
			self.assertEquals(l, values)

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
		

if __name__ == "__main__":
	pylot.enterBenchMode()

	suite = unittest.TestSuite(map(unittest.TestLoader().loadTestsFromTestCase,
		(TestWritingVarArgs, TestReadingVarArgs)
	))
	
	stream = BlackHole() if pylot.mpi_rank != 0 else sys.stderr
	unittest.TextTestRunner(stream=stream, verbosity=2).run(suite)

	pylot.exitBenchMode()
	
