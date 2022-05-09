cmake_minimum_required(VERSION 3.19)
project(MoFS)

set(CMAKE_CXX_STANDARD 14)

add_executable(MoFS
        main.cpp
        include/MemInode.h fs/MemInode.cpp
        include/SuperBlock.h fs/SuperBlock.cpp
        include/DirEntry.h
        include/device/DeviceManager.h fs/device/DeviceManager.cpp
        include/OpenFile.h fs/OpenFile.cpp
        utils/Diagnose.h utils/Diagnose.cpp
        include/User.h fs/User.cpp
        include/DiskInode.h fs/DiskInode.cpp
        include/Primitive.h fs/Primitive.cpp
        include/MoFSErrno.h fs/MoFSErrno.cpp)
