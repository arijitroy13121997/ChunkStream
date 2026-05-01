cmake_minimum_required(VERSION 3.10)
project(ChunkStream)

set(CMAKE_CXX_STANDARD 17)

find_package(OpenSSL REQUIRED)
find_package(nlohmann_json REQUIRED)

add_definitions(-DCONFIG_PATH="${CMAKE_CURRENT_SOURCE_DIR}/cfg/config.json")

include_directories(inc)

add_executable(sender src/sender.cpp src/socket_utils.cpp)
add_executable(receiver src/receiver.cpp src/socket_utils.cpp)

target_link_libraries(sender OpenSSL::Crypto nlohmann_json::nlohmann_json)
target_link_libraries(receiver OpenSSL::Crypto nlohmann_json::nlohmann_json)

configure_file(cfg/data.txt data.txt COPYONLY)