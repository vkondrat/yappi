import time
import yappi
import cProfile

def a_very_long_function_name():
	pass

def state(entry):
	print entry

def foo():
	time.sleep(1)


cProfile.run('foo()', 'fooprof')
import pstats
p = pstats.Stats('fooprof')
p.strip_dirs().sort_stats(-1).print_stats()




yappi.start(True)
foo()
a_very_long_function_name()
yappi.stop()yappi.enum_stats(state)
li = yappi.get_stats(1, 1, 8)
for it in li:
	print it
yappi.clear_stats()
