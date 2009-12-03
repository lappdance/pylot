import unittest
import sys
import utils

sys.path.append("..")
import pylot

toEchoer = None
fromEchoer = None

def echoOne(i, j):
	global toEchoer, fromEchoer
	
	pylot.write(fromEchoer, pylot.read(toEchoer))

class TestWritingBasicTypes(unittest.TestCase):
	def setUp(self):
		global toEchoer, fromEchoer
	
		pylot.configure()
		echoproc = pylot.createProcess(echoOne, 0, None)

		toEchoer = pylot.createChannel(None, echoproc)
		fromEchoer = pylot.createChannel(echoproc, None)

		self.rank = pylot.startAll()
	
	def tearDown(self):
		if self.rank == 0:
			pylot.stopMain(0)
	
	def testInt(self):
		if self.rank == 0:
			self.sendToEchoer(42)

	def testFloat(self):
		if self.rank == 0:
			self.sendToEchoer(6.67428e-11)
	
	def testBool(self):
		if self.rank == 0:
			self.sendToEchoer(True)
	
	def testNone(self):
		if self.rank == 0:
			self.sendToEchoer(None)
	
	def testString(self):
		if self.rank == 0:
			self.sendToEchoer("words words words")

	def testListOfInts(self):
		if self.rank == 0:
			l = [1, 2, 3]
			self.sendToEchoer(l)
	
	def testTupleOfChars(self):
		if self.rank == 0:
			t = ('a', 'b')
			self.sendToEchoer(t)

	def testListOfDifferentTypes(self):
		if self.rank == 0:
			l = [1, None, 6.7, "nothing"]
			self.sendToEchoer(l)

	def testListOfTuples(self):
		if self.rank == 0:
			l = [(1, "ab"), (None, 6.5)]
			self.sendToEchoer(l)

	def testDict(self):
		if self.rank == 0:
			d = { "key" : "value" }
			self.failToSendToEchoer(d, TypeError)
	
	def testStruct(self):
		if self.rank == 0:
			class C:
				pass
		
			c = C()
			c.index = 0

			self.failToSendToEchoer(c, TypeError)
	
	def sendToEchoer(self, data):
		global toEchoer, fromEchoer
		pylot.write(toEchoer, data)
		
		echo = pylot.read(fromEchoer)	
		self.assertEqual(data, echo)
	
	def failToSendToEchoer(self, data, expectedError):
		global toEchoer, fromEchoer
		
		self.assertRaises(expectedError, pylot.write, toEchoer, data)
		
		#the reader process is still waiting for us to send it something
		#so just give it a null
		pylot.write(toEchoer, None)
		#the echoer is going to echo the value back, and it'll be left in the
		#channel if we don't clear it out
		#stopmain() is called at the end of this test, which deletes the channel,
		#but deleting the Pilot channel doesn't affect the MPI communication
		#layer
		pylot.read(fromEchoer)


def echoMany(i, j):
	global toEchoer, fromEchoer
	
	n = pylot.read(toEchoer)
	pylot.write(fromEchoer, *pylot.read(toEchoer, n))

class TestWritingVarArgs(unittest.TestCase):
	def setUp(self):
		global toEchoer, fromEchoer
	
		pylot.configure()
		echoproc = pylot.createProcess(echoMany, 0, None)

		toEchoer = pylot.createChannel(pylot.MAIN, echoproc)
		fromEchoer = pylot.createChannel(echoproc, pylot.MAIN)

		self.rank = pylot.startAll()
	
	def tearDown(self):
		if self.rank == 0:
			pylot.stopMain(0)
	
	def testOneItemWithoutVarArgs(self):
		if self.rank == 0:
			global toEchoer, fromEchoer
			
			pylot.write(toEchoer, 1)
			pylot.write(toEchoer, 6.7)
			values = pylot.read(fromEchoer, 1)
	
			self.assertEqual(1, len(values))
			self.assertEqual(6.7, values[0])
	
	def testOneItemWithVarArgs(self):
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
			
			l = [3.1, 6.4, 2.9]
			pylot.write(toEchoer, 3, *l)
			values = pylot.read(fromEchoer, 3)
			
			self.assertEquals(3, len(values))
			self.assertEquals(l, values)

if __name__ == "__main__":
	pylot.enterBenchMode()
	pylot.globals.PI_QuietMode = 1
	
	suite = unittest.TestSuite(map(unittest.TestLoader().loadTestsFromTestCase,
		(TestWritingBasicTypes, TestWritingVarArgs)
	))
	
	stream = utils.BlackHole() if pylot.mpi_rank != 0 else sys.stderr
	unittest.TextTestRunner(stream=stream, verbosity=2).run(suite)

	pylot.exitBenchMode()
	
