cmake_minimum_required(VERSION 3.10)
project(ChunkStream)
enable_testing()

set(CMAKE_CXX_STANDARD 17)

find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_definitions(-DCONFIG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/cfg/config.json")

include_directories(inc)

add_executable(sender src/sender.cpp src/socket_utils.cpp src/checksum_helper.cpp)
add_executable(receiver src/receiver.cpp src/handle_transfer.cpp src/socket_utils.cpp src/checksum_helper.cpp)

target_link_libraries(sender OpenSSL::Crypto nlohmann_json::nlohmann_json)
target_link_libraries(receiver OpenSSL::Crypto nlohmann_json::nlohmann_json)

if (WIN32)
    target_link_libraries(sender ws2_32)
    target_link_libraries(receiver ws2_32)
endif()

configure_file(cfg/data.txt data.txt COPYONLY)

# Tests
if (NOT WIN32)
add_executable(chunkstream_test 
    tests/gtest.cpp
    src/handle_transfer.cpp
    src/socket_utils.cpp
    src/checksum_helper.cpp
)   
target_link_libraries(chunkstream_test 
                        gtest 
                        pthread 
                        OpenSSL::Crypto
                        nlohmann_json::nlohmann_json)

add_test(NAME ChunkStreamTest COMMAND chunkstream_test)
endif()
