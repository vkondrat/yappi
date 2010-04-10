import time 
import yappi 
import cProfile 

def a_very_long_function_name(): pass 

def foo(): pass

t0 = time.time()
cProfile.run('foo()', 'fooprof') 
print "Elapsed %0.6f" % (time.time()-t0)
import pstats 
p = pstats.Stats('fooprof') 
p.strip_dirs().sort_stats(-1).print_stats() 

t0 = time.time()
yappi.start()
foo()
yappi.stop()
print "Elapsed %0.6f" % (time.time()-t0)
yappi.print_stats()
