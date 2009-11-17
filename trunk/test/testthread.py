import time
import yappi
import threading

def estat(entry):
	pass

class myclass(threading.Thread):
	def myfunc(self):
		pass
	def run(self):
		self.myfunc()
		time.sleep(0.001)

if __name__ == "__main__":
	yappi.start(True)
	myt = myclass()	
	myt.start()
	myt.join()
	yappi.enum_stats(estat)
	myt = myclass()	
	myt.start()
	myt.join()
	yappi.stop()
	yappi.enum_stats(estat)
	yappi.print_stats()
	yappi.clear_stats()
	yappi.start(True)
	myt = myclass()	
	myt.start()
	myt.join()
	yappi.stop()
	yappi.print_stats(yappi.SORTTYPE_NAME, yappi.SORTORDER_ASCENDING)
	yappi.clear_stats()
	
