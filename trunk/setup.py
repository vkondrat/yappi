from distutils.core import setup, Extension

setup(name="_yappi", version="0.4 beta",
	ext_modules = [Extension("_yappi", 
		sources = ["_yappi.c", "_ycallstack.c", "_yhashtab.c", "_ymem.c", "_yfreelist.c"],
		#extra_compile_args = ["-D DEBUG_MEM", "-D DEBUG_CALL", "-D YDEBUG"],
		#extra_compile_args = ["-D YDEBUG"],
		#extra_compile_args = ["-D DEBUG_CALL"],
		#extra_compile_args = ["-D DEBUG_MEM"],
		#extra_link_args = ["-lrt"]
	)],
	py_modules = ["yappi"])
