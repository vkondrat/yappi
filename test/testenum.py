import yappi

def es(entry):
	print "%.20s, %ld, %0.4f, %0.4f" % (entry[0], entry[1], entry[2], entry[3])
def foo():
	pass
yappi.start()
foo()
yappi.enum_stats(es)
yappi.print_stats()
