cmake_minimum_required(VERSION 3.8)

project(threadpool)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(threadpool src/threadpool.cpp)

target_include_directories(threadpool PUBLIC include)

#set(test_sources
#    testsrc/tp_test.cpp
#   )

#add_executable(tp_test ${test_sources})

#target_include_directories(tp_test PUBLIC include)

#target_link_libraries(tp_test threadpool)
