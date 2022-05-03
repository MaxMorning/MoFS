/**
 * @file DiskInode.cpp
 * @brief 外存inode实现文件
 * @mainpage DiskInode
 * @author 韩孟霖
 * @date 2022/05/03
 * @license GPL v3
 */
#include <cstring>

#include "../include/DiskInode.h"

/**
 * @brief DiskInode构造函数
 */
DiskInode::DiskInode() {
    this->d_mode = 0;
    this->d_nlink = 0;
    this->d_uid = -1;
    this->d_gid = -1;
    this->d_size = 0;

    memset(this->d_addr, 0, 10 * sizeof(int));

    this->d_atime = 0;
    this->d_mtime = 0;
}

/**
 * @brief DiskInode析构函数
 */
DiskInode::~DiskInode() = default;
