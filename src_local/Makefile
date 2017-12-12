TOP_PATH:=../
OUTDIR=$(TOP_PATH)/build/
SRCDIR=$(TOP_PATH)/src_local/

DLIBS = -lopencv_core2413 -lopencv_imgproc2413 -lopencv_highgui2413
INCLUDE_PATH=-I$(TOP_PATH)/3rd/opencv/mingw/include/ 

INCLUDE_PATH_LOCAL=-I$(TOP_PATH)/include
LIB_PATH=-L$(TOP_PATH)/3rd/opencv/mingw/x86/mingw/lib
.PHONY: bb  
#mingw32-g++不能写注释
all: cvLaplace
rotate:
	g++   -g  -o  $(OUTDIR)rotate  $(SRCDIR)rotate.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
BlobAnalysisSuzuki:
	g++   -g  -o  $(OUTDIR)BlobAnalysisSuzuki  $(SRCDIR)BlobAnalysisSuzuki.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
cvLaplace:
	g++   -g  -o  $(OUTDIR)cvLaplace  $(SRCDIR)cvLaplace.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
soble:
	g++   -g  -o  $(OUTDIR)soble  $(SRCDIR)soble.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
bilateralFilter:
	g++   -g  -o  $(OUTDIR)bilateralFilter  $(SRCDIR)bilateralFilter.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
prydown:
	g++ -g  -o  $(OUTDIR)prydown  $(SRCDIR)prydown.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
adaptiveThreshold:
	g++ -g  -o  $(OUTDIR)adaptiveThreshold  $(SRCDIR)adaptiveThreshold.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
box:
	g++ -g  -o  $(OUTDIR)box  $(SRCDIR)box.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)		
line2:
	g++ -g  -o  $(OUTDIR)line2  $(SRCDIR)line2.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
line1:
	g++ -g  -o  $(OUTDIR)line1  $(SRCDIR)line1.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)	
all1:
	g++ -g  -o  $(OUTDIR)fill  $(SRCDIR)fill.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
	g++ -g  -o  $(OUTDIR)houghlines  $(SRCDIR)houghlines.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
	g++ -g  -o  $(OUTDIR)erode  $(SRCDIR)erode.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
	g++ -g  -o  $(OUTDIR)dilate  $(SRCDIR)dilate.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
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
HoughChange:
	g++   -g  -o  $(OUTDIR)HoughChange  $(SRCDIR)HoughChange.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
	g++ -g  -o  $(OUTDIR)test  $(SRCDIR)test.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)
morphology:
	g++ -g  -o  $(OUTDIR)morphology  $(SRCDIR)morphology.cpp   $(DLIBS) $(INCLUDE_PATH) $(LIB_PATH)