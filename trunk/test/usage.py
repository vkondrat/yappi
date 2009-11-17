import time
import threading
import yappi
	
def foo():
    time.sleep(0.1)

def profileme():
    for i in range(5):
        mythread = t()
        mythread.start()
        mythread.join()
    foo()

class t(threading.Thread):
    def run(self):
        self.foo2()
    def foo2(self):
        time.sleep(1.0)

if __name__ == "__main__":
	yappi.start(True)
	profileme()
	yappi.stop()
	yappi.print_stats(2)
