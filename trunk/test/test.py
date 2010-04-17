import time 
import yappi 
import cProfile 

def a_very_long_function_name(): pass 

def bar(): pass

def foo():
	i = 1000000
	while(i):
		bar()
		i -= 1

t0 = time.time()
foo()
print " >>>>>>>>> No profiler Elapsed %f" % (time.time()-t0)

t0 = time.time()
cProfile.run('foo()', 'fooprof') 
print " >>>>>>>>> cProfile Elapsed %f" % (time.time()-t0)
import pstats 
p = pstats.Stats('fooprof') 
p.strip_dirs().sort_stats(-1).print_stats() 

t0 = time.time()
yappi.start(timing_sample=1)
foo()
yappi.stop()
print " >>>>>>>>> Yappi Elapsed %f" % (time.time()-t0)
yappi.print_stats()
