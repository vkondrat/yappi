import yappi
import time
from threading import Thread

def test1():
	time.time()

class test(Thread):
	def test2(self):
		pass
	def run(self):
		self.test2()
		time.sleep(2)


#a = test()
#a.start()
#b = test()
#b.start()
#a.test2()
#test1()
#time.sleep(1)

def foo5():
	time.sleep(1)

def foo4():
	time.sleep(1)
	

def foo3():
	time.sleep(1)
	foo4()
	foo5()

def foo2():
	time.sleep(1)
	foo3()
	

def foo():
	time.sleep(1)
	B = test()
	B.start()
	c = test()
	c.start()
	for i in range(1000):
	    test1()
	for i in range(2):
	    foo2()

def estat(entry):
	print entry	

yappi.start(True)
print 'PROFILER STARTED...'

time.sleep(1)
B = test()
B.start()
c = test()
yappi.enum_stats(estat)
c.start()
test1()
foo()


	

for i in range(2):
	foo2()
yappi.enum_stats(estat)
yappi.stop()
yappi.print_stats()
yappi.clear_stats()

'''
import cProfile
cProfile.run('foo()', 'fooprof')
import pstats
p = pstats.Stats('fooprof')
p.strip_dirs().sort_stats(-1).print_stats()
'''

