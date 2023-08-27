cmake_minimum_required(VERSION 3.10)

project(MultiFileHandler)

set(CMAKE_CXX_STANDARD 11)

set(Boost_USE_STATIC_LIBS OFF)
set(Boost_USE_MULTITHREADED ON)
set(Boost_USE_STATIC_RUNTIME OFF)
# set(BOOST_ROOT /path/to/boost_1_80_0)

find_package(Boost 1.80.0 REQUIRED COMPONENTS log log_setup system thread)

add_executable(Requester src/Requester.cpp)
target_link_libraries(Requester rt pthread)

add_executable(FileHandler src/FileHandler.cpp)
target_link_libraries(FileHandler rt pthread Boost::log Boost::log_setup Boost::system Boost::thread)