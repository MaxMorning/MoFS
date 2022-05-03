cmake_minimum_required(VERSION 3.21)
project(MoFS)

set(CMAKE_CXX_STANDARD 14)

add_executable(MoFS
        main.cpp fs/DiskInode.cpp include/MemInode.h fs/MemInode.cpp include/SuperBlock.h include/DirEntry.h include/device/DeviceManager.h include/File.h fs/device/DeviceManager.cpp utils/Diagnose.cpp utils/Diagnose.h fs/SuperBlock.cpp)
