python25 setup.py clean build --compiler=mingw32 install 
python25 test/testenum.py
python25 test/test.py
python setup.py clean build install
python test/test.py
pause 