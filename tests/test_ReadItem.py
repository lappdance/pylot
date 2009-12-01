import unittest
import sys

sys.path.append("..")
import pylot

toEchoer = None
fromEchoer = None

def echoer(i, j):
	global toEchoer, fromEchoer
	
	pylot.write(fromEchoer, pylot.read(toEchoer))

class TestBasicTypes(unittest.TestCase):
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
	
	def testInt(self):
		if self.rank == 0:
			global toEchoer, fromEchoer
			sent = 42
			pylot.write(toEchoer, sent)
			echo = pylot.read(fromEchoer)
		
			self.assertEqual(sent, echo, "expected {0} but got {1}".format(sent, echo))

	def testFloat(self):
		if self.rank == 0:
			global toEchoer, fromEchoer
			sent = 6.67428e-11
			pylot.write(toEchoer, sent)
			echo = pylot.read(fromEchoer)
		
			self.assertAlmostEqual(sent, echo, 7, "expected {0} but got {1}".format(sent, echo))
	
	def testBool(self):
		if self.rank == 0:
			global toEchoer, fromEchoer
			sent = True
			pylot.write(toEchoer, sent)
			echo = pylot.read(fromEchoer)
		
			self.assertEqual(sent, echo, "expected {0} but got {1}".format(sent, echo))
	
	def testNone(self):
		if self.rank == 0:
			global toEchoer, fromEchoer
			sent = None
			pylot.write(toEchoer, sent)
			echo = pylot.read(fromEchoer)
		
			self.assertEqual(sent, echo, "expected {0} but got {1}".format(sent, echo))
	
	def testString(self):
		if self.rank == 0:
			global toEchoer, fromEchoer
			sent = "words words words"
			pylot.write(toEchoer, sent)
			echo = pylot.read(fromEchoer)
		
			self.assertEqual(sent, echo, "expected {0} but got {1}".format(sent, echo))

	def testListOfInts(self):
		if self.rank == 0:
			global toEchoer, fromEchoer
			sent = [1, 2, 3]
			pylot.write(toEchoer, sent)
			echo = pylot.read(fromEchoer)
		
			self.assertEqual(sent, echo, "expected {0} but got {1}".format(sent, echo))
			self.assertEqual(type(sent), type(echo), "expected {0} but got {1}".format(type(sent), type(echo)))
	
	def testTupleOfChars(self):
		if self.rank == 0:
			global toEchoer, fromEchoer
			sent = ('a', 'b')
			pylot.write(toEchoer, sent)
			echo = pylot.read(fromEchoer)
		
			self.assertEqual(sent, echo, "expected {0} but got {1}".format(sent, echo))
			self.assertEqual(type(sent), type(echo), "expected {0} but got {1}".format(type(sent), type(echo)))

	def testListOfDifferentTypes(self):
		if self.rank == 0:
			global toEchoer, fromEchoer
			sent = [1, None, 6.7, "nothing"]
			pylot.write(toEchoer, sent)
			echo = pylot.read(fromEchoer)
		
			self.assertEqual(sent, echo, "expected {0} but got {1}".format(sent, echo))

	def testListOfTuples(self):
		if self.rank == 0:
			global toEchoer, fromEchoer
			sent = [(1, 2), (5, 6)]
			pylot.write(toEchoer, sent)
			echo = pylot.read(fromEchoer)
		
			self.assertEqual(sent, echo, "expected {0} but got {1}".format(sent, echo))

	def testDict(self):
		if self.rank == 0:
			global toEchoer, fromEchoer
			sent = { "key" : "value" }
			self.assertRaises(TypeError, pylot.write, toEchoer, sent)
			
			#the reader process is still waiting for us to send it something
			#so just give it a null
			pylot.write(toEchoer, None)
			#the echoer is going to echo the value back, and it'll be left in the
			#channel if we don't clear it out
			#stopmain() is called at the end of this test, which deletes the channel,
			#but deleting the Pilot channel doesn't affect the MPI communication
			#layer
			pylot.read(fromEchoer)
	
	def testStruct(self):
		if self.rank == 0:
			global toEchoer, fromEchoer
			class C:
				pass
		
			sent = C()
			sent.index = 0

			self.assertRaises(TypeError, pylot.write, toEchoer, sent)
			
			pylot.write(toEchoer, None)
			pylot.read(fromEchoer)

class BlackHole:
	def write(self, *args):
		pass
	def flush(self):
		pass

if __name__ == "__main__":
	pylot.enterBenchMode()
	pylot.globals.PI_QuietMode = 1
	
	suite = unittest.TestLoader().loadTestsFromTestCase(TestBasicTypes)
	stream = BlackHole() if pylot.mpi_rank != 0 else sys.stderr
	unittest.TextTestRunner(stream=stream, verbosity=2).run(suite)

	pylot.exitBenchMode()
	
