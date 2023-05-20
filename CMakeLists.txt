cmake_minimum_required(VERSION 3.25)
project(luno_cpp)

set(CMAKE_CXX_STANDARD 23)

add_executable(luno-cpp luno-cpp/assert.cpp luno-cpp/luno.cpp luno-cpp/preprocessor_states.cpp luno-cpp/token.cpp)
target_include_directories(luno-cpp PUBLIC .)
