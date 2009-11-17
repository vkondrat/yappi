import time
import cProfile
import yappi

class g:
	stop = 0

def foo3():
	time.sleep(0.0001)
	foo()

def foo2():
    	time.sleep(0.0001)
 	foo3()

def foo():
    	if g.stop == 140:
		return
	g.stop += 1    	
	foo2()    	
    	foo()
def averylongfunctionname_will_be_just_like_this():
	pass

def estat(entry):
	print '%s     %i     %0.7f     %0.7f' % (entry[0], entry[1], entry[2], entry[3])

cProfile.run('foo()', 'fooprof')
import pstats
p = pstats.Stats('fooprof')
p.strip_dirs().sort_stats(-1).print_stats()

yappi.start(True)
averylongfunctionname_will_be_just_like_this()
g.stop = 0
foo()
print "name             callcnt    #ttot	#tsub"
print "----------------------------------------------"
yappi.enum_stats(estat)
averylongfunctionname_will_be_just_like_this()
yappi.print_stats()
yappi.stop()
yappi.clear_stats()
