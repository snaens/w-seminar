cmake_minimum_required(VERSION 3.26)
project(FdPB)

set(CMAKE_CXX_STANDARD 14)

find_package(OpenCV REQUIRED)

include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(FdPB
        BOS.cpp)

target_link_libraries(FdPB ${OpenCV_LIBS})
