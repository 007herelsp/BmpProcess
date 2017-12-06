OUTDIR=./build/
SRCDIR=./src/
DLIBS = -lopencv_core2413 -lopencv_imgproc2413 -lopencv_highgui2413
INCLUDE_PATH = -ID:/opencv_mingw/install/include/
LIB_PATH = -LD:/opencv_mingw/install/x86/mingw/lib  
#mingw32-g++不能写注释
all:
	g++ -g  -o  $(OUTDIR)hist  $(SRCDIR)hist.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
	g++ -g  -o  $(OUTDIR)laplc  $(SRCDIR)laplc.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
	g++ -g  -o  $(OUTDIR)gamma  $(SRCDIR)gamma.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
	g++ -g  -o  $(OUTDIR)test4  $(SRCDIR)test4.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
	g++ -g  -o  $(OUTDIR)test3  $(SRCDIR)test3.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
	g++ -g  -o  $(OUTDIR)test2  $(SRCDIR)test2.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
	g++  -std=c++11   -g -c -o $(OUTDIR)process.o $(SRCDIR)process.cpp
	g++  -std=c++11   -g -c -o $(OUTDIR)c_canny.o $(SRCDIR)c_canny.cpp
	g++  -std=c++11   -g -c -o $(OUTDIR)main.o $(SRCDIR)main.cpp $(INCLUDE_PATH)
	g++ -o  $(OUTDIR)main $(OUTDIR)process.o $(OUTDIR)main.o   $(DLIBS) $(LIB_PATH)
	g++   -g -c -o $(OUTDIR)process.o $(SRCDIR)process.cpp
	g++ -g  -o  $(OUTDIR)HoughChange  $(SRCDIR)HoughChange.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
	g++ -g  -o  $(OUTDIR)test  $(SRCDIR)test.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)