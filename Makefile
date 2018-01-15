CC = gcc 
CXX = g++  
LD = g++ 
INCLUDE_PATH=./include
CFLAGS = -Wall -g  -pipe  -c  -I$(INCLUDE_PATH)
CXXFLAGS  = -Wall -g -g3 -ggdb -std=c++11 -pipe  -c  -I$(INCLUDE_PATH)
#LDFLAGS =   -lpthread    -ldl   -lrt
#-lapr-1 for libapr
OUT_DIR = ./build
SRCS = $(wildcard *.cpp src/*.cpp)  
OBJS = $(patsubst %.cpp, $(OUT_DIR)/%.o, $(notdir ${SRCS}))  

SRC_DIR = ./src
TARGET = Composer  
 
.PHONY: all install clean 
rebuild: clean all 
BIN_TARGET = ${OUT_DIR}/${TARGET}
all: $(BIN_TARGET)
$(BIN_TARGET): $(OBJS)
	$(LD) -o $@ $^ $(LDFLAGS)
$(OUT_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CXX) -o $@ $<	$(CXXFLAGS)
$(OUT_DIR)/%.o:$(SRC_DIR)/%.c  
	$(CC)  -o $@ $<	$(CFLAGS)

install:
	mkdir -p ${OUT_DIR}
	cp -f $(TARGET) $(OUT_DIR)
	cp -rf $(CONF_DIR) $(OUT_DIR)
	cp -f auto.sh $(OUT_DIR)

clean:
	rm -f $(OUT_DIR)/*.o $(BIN_TARGET)  


testcase0:
	$(BIN_TARGET) ./images/test0.bmp ./images/Target17.bmp ./images/Source1_1.bmp ./images/Source1_2.bmp ./images/Source1_3.bmp ./images/Source1_4.bmp ./images/Source1_5.bmp ./images/Source1_6.bmp
testcase1:
	$(BIN_TARGET) ./images/test1.bmp ./images/Target19.bmp ./images/Source1_1.bmp ./images/Source1_2.bmp ./images/Source1_3.bmp ./images/Source1_4.bmp ./images/Source1_5.bmp ./images/Source1_6.bmp
testcase2:
	$(BIN_TARGET) ./images/test2.bmp ./images/Target20.bmp ./images/Source1_1.bmp ./images/Source1_2.bmp ./images/Source1_3.bmp ./images/Source1_4.bmp ./images/Source1_5.bmp ./images/Source1_6.bmp

#
#check:
#	scan-build make

 