OUTDIR=./build/
SRCDIR=./src/
all:
	g++    -g -c -o $(OUTDIR)process.o $(SRCDIR)process.cpp
	g++    -g -c -o $(OUTDIR)c_canny.o $(SRCDIR)c_canny.cpp
	g++    -g -c -o $(OUTDIR)main.o $(SRCDIR)main.cpp
	g++ -o $(OUTDIR)main $(OUTDIR)process.o $(OUTDIR)main.o