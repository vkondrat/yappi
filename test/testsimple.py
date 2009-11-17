import yappi

def estat(e):
	print e
def foo():
	pass
yappi.start(True)
for i in xrange(100000):
	foo()
yappi.stop()
#yappi.enum_stats(estat)
yappi.print_stats()
yappi.clear_stats()
