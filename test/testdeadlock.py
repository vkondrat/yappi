import yappi
import threading
class deadlock_thread(threading.Thread):
    # acquire the lock two times to create a very
    # simple deadlock:)
    def foo(self):
		for i in xrange(0,2): self.lock.acquire()
    def run(self):
        self.lock = threading.Lock()
        self.foo()
yappi.start() # start the profiler
thr = deadlock_thread() # start the deadlock thread
thr.start()
while 1:
    s = raw_input("yappi>")
    exec s
