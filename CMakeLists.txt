cmake_minimum_required(VERSION 3.21)
project(MoFS)

set(CMAKE_CXX_STANDARD 14)

add_executable(MoFS
        main.cpp fs/DiskInode.cpp)
