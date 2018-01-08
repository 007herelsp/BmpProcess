# 用来构建该项目所用cmake的最低版本。
cmake_minimum_required(VERSION 2.6)

# avoid some cmake warnings
IF(POLICY CMP0026)
    CMAKE_POLICY(SET CMP0026 OLD)
ENDIF()
# 项目名
project (BMP)
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")
include_directories ("./include")
AUX_SOURCE_DIRECTORY("./src/" DIR_SRCS)
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
ADD_EXECUTABLE(${PROJECT_NAME} ${DIR_SRCS})
