import threading
import time
import yappi

def es(e):
	pass

def func1():
	time.sleep(0.02)

class thread_a(threading.Thread):
	def thread_func1(self):
		pass
	def run(self):
		self.thread_func1()
		time.sleep(0.01)
try:
	yappi.stop()
except Exception,e:
	print e
try:
	yappi.clear_stats()
except Exception,e:
	print e
try:
	yappi.enum_stats(es)
except Exception,e:
	print e
try:
	yappi.print_stats()
except Exception,e:
	print e
yappi.start(True)
try:
	yappi.start(True)
except Exception,e:
	print e
t = thread_a()
t.start()
t.join()
func1()
yappi.enum_stats(es)
yappi.print_stats()
yappi.stop()
yappi.enum_stats(es)
yappi.print_stats()
yappi.clear_stats()
try:
	yappi.print_stats()
except Exception,e:
	print e
try:	
	yappi.enum_stats(es)
except Exception,e:
	print e
