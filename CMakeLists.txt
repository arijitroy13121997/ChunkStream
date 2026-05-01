cmake_minimum_required(VERSION 3.10)
project(ChunkStream)

set(CMAKE_CXX_STANDARD 17)

add_executable(sender src/sender.cpp )
add_executable(receiver src/receiver.cpp )