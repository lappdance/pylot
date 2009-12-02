import unittest
import sys
import utils
import math

sys.path.append("..")
import pylot

class ChannelInfo:
	pass

def echoSingle(index, channelInfo):
	pylot.write(channelInfo.out_, pylot.read(channelInfo.in_))

def echoMany(index, data):
	n = pylot.read(channelInfo.in_)
	pylot.write(channelInfo.out_, *pylot.read(channelInfo.in_, n))

class TestBroadcastSingle(unittest.TestCase):
	def setUp(self):
		global toES
		
		self.in_ = []
		self.out_ = []
		
		pylot.configure()
		
		for n in range(pylot.mpi_worldsize - 1):
			info = ChannelInfo()
			
			echoer = pylot.createProcess(echoSingle, n, info)
			out = pylot.createChannel(None, echoer)
			self.out_.append(out)
			
			in_ = pylot.createChannel(echoer, None)
			self.in_.append(in_)
			
			info.in_ = out
			info.out_ = in_

		self.echoers = pylot.createBundle(pylot.BROADCAST, self.out_)
		
		self.rank = pylot.startAll()
	
	def tearDown(self):
		if self.rank == 0:
			pylot.stopMain(0)
	
	def testSendInt(self):
		if self.rank == 0:
			self.sendToEchoer(42)
	
	def testSendFloat(self):
		if self.rank == 0:
			self.sendToEchoer(math.e)
	
	def testSendBool(self):
		if self.rank == 0:
			self.sendToEchoer(True)
	
	def testSendNone(self):
		if self.rank == 0:
			self.sendToEchoer(None)
	
	def testSendString(self):
		if self.rank == 0:
			self.sendToEchoer("lorem ipsum dolor sit amet")

	def testSendList(self):
		if self.rank == 0:
			l = [1, "bee", 3.14, None]
			self.sendToEchoer(l)
	
	def testSendDict(self):
		if self.rank == 0:
			d = {"key" : "value"}
			self.failToSendToEchoer(d, TypeError)
	
	def testSendStruct(self):
		if self.rank == 0:			
			self.failToSendToEchoer(self, TypeError)

	def sendToEchoer(self, data):		
		global fromES
		pylot.broadcast(self.echoers, data)
		
		for echoer in self.in_:
			echo = pylot.read(echoer)	
			self.assertEqual(data, echo, "expected {0} but got {1}".format(data, echo))
	
	def failToSendToEchoer(self, data, expectedError):
		self.assertRaises(expectedError, pylot.broadcast, self.echoers, data)
		
		#the reader process is still waiting for us to send it something
		#so just give it a null
		pylot.broadcast(self.echoers, None)
		#the echoer is going to echo the value back, and it'll be left in the
		#channel if we don't clear it out
		#stopmain() is called at the end of this test, which deletes the channel,
		#but deleting the Pilot channel doesn't affect the MPI communication
		#layer
		for echoer in self.in_:
			pylot.read(echoer)

class TestBroadcastMultiple(unittest.TestCase):
	pass

if __name__ == "__main__":
	pylot.enterBenchMode()
	pylot.globals.PI_QuietMode = 1
	
	import os, time
	print os.getpid(), pylot.mpi_rank
#	time.sleep(19)
	
	suite = unittest.TestLoader().loadTestsFromTestCase(TestBroadcastSingle)
	stream = utils.BlackHole() if pylot.mpi_rank != 0 else sys.stderr
	unittest.TextTestRunner(stream=stream, verbosity=2).run(suite)

	pylot.exitBenchMode()

