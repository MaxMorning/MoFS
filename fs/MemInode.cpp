/**
 * @file MemInode.cpp
 * @brief 内存Inode实现
 * @author 韩孟霖
 * @date 2022/5/3
 * @license GPL v3
 */
#include <cstring>
#include <ctime>

#include "../include/MemInode.h"

MemInode::MemInode(const DiskInode &diskInode, int diskInodeIdx) {
    this->i_flag = 0;
    this->i_mode = diskInode.d_mode;

    this->i_count = 0;
    this->i_nlink = diskInode.d_nlink;

    this->i_dev = 0; // 默认设置设备号为0
    this->i_number = diskInodeIdx;

    this->i_uid = diskInode.d_uid;
    this->i_gid = diskInode.d_gid;

    this->i_size = diskInode.d_size;
    memcpy(this->i_addr, diskInode.d_addr, 10 * sizeof(int));

    this->i_lastr = -1;
}
