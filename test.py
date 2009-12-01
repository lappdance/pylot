#!/usr/bin/python
#mpiexec -np 2 python test.py

import sys
import pylot
import os
import time
import random

toWorker = None
toMain = []

def doWork(rank, nothing):
	global toWorker

	print pylot.read(toWorker) #(1, 2)
	print pylot.read(toWorker, 2) #[3, "words"] #this is broken
	print pylot.read(toWorker) #[1, 2, 3]
	print pylot.read(toWorker) #([1, 2], 99, None)
	print pylot.read(toWorker, 4) #[1, 2, 3, "anything"]
	print pylot.read(toWorker, 2) #["nothing", (9, "gg")]

def writer(rank, data):
	global toMain
	
	timeout = random.randint(1, 4)
	time.sleep(timeout)
	
	pylot.write(toMain[rank], rank)

def main():
	global toWorker
	global toMain
	
	N = pylot.configure(sys.argv)
	N -= 1

	worker = pylot.createProcess(doWork, 0, "nothing")
	toWorker = pylot.createChannel(pylot.MAIN, worker)
	N -= 1
	
	for i in range(N):
		w = pylot.createProcess(writer, i, None)
		toMain.append(pylot.createChannel(w, pylot.MAIN))
	
	b = pylot.createBundle(pylot.SELECT, toMain)

	pylot.startAll()

	pylot.write(toWorker, (1, 2), 3) #tuple, arg
	pylot.write(toWorker, "words", [1, 2, 3]) #string, list(3)
	pylot.write(toWorker, ([1, 2], 99, None)) #tuple(list(3), int, none)
	pylot.write(toWorker, 1, 2, 3, "anything", "nothing", (9, "gg"))

	idx = pylot.select(b)
	print "ready to read from", idx
	channel = pylot.getBundleChannel(b, idx)

	print pylot.read(channel)

	pylot.stopMain(0)

def foo():
	pylot.createProcess(pylot.endTime, 0, 0)

if __name__ == "__main__":
	main()

