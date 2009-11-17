import time
import yappi
from threading import Thread

NTHREAD = 5
MAXRDEPTH = 10

def foo2(): 
	time.sleep(0.1)

def foo(rdepth):
	if (rdepth == MAXRDEPTH):
		return
	foo2()
	foo(rdepth+1)	

if __name__ == "__main__":
	yappi.start(True) # start profiler with built-in profiling set to True
	for i in range(NTHREAD): # multiple threads calling a recursive function.
		thr = Thread(foo(1)); thr.start(); 	thr.join()
	for it in yappi.get_stats(yappi.SORTTYPE_TTOTAL,
							  yappi.SORTORDER_ASCENDING,
							  yappi.SHOW_ALL):
		print it