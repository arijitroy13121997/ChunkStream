cmake_minimum_required(VERSION 3.10)
project(ChunkStream)

set(CMAKE_CXX_STANDARD 17)

include_directories(inc)

add_executable(sender src/sender.cpp src/socket_utils.cpp)
add_executable(receiver src/receiver.cpp src/socket_utils.cpp)