cmake_minimum_required(VERSION 3.17)
project(jvmAttach)

set(CMAKE_CXX_STANDARD 14)

add_executable(jvmAttach main.cpp AttachListener.cpp AttachListener.h AttachOperation.cpp AttachOperation.h)