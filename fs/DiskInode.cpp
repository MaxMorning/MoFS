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
#include "../include/device/DeviceManager.h"
#include "../include/SuperBlock.h"

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

int DiskInode::DiskInodeFactory(int diskInodeIdx, DiskInode &diskInode) {
    /// @todo 使用缓存加速访问
    if (DeviceManager::deviceManager.ReadInode(diskInodeIdx, &diskInode) == -1) {
        return -1;
    }

    return 0;
}

int DiskInode::ReleaseBlocks() {
    int blockNum = (this->d_size - 1) / BLOCK_SIZE + 1;

    for (int i = 0; i < 6; ++i) {
        if (this->d_addr[i] > 0) {
            SuperBlock::superBlock.ReleaseBlock(this->d_addr[i]);
        }
    }

    int buffer[128];
    for (int i = 6; i < 8; ++i) {
        if (this->d_addr[i] > 0) {
            // 一级索引有效
            if (BLOCK_SIZE != DeviceManager::deviceManager.ReadBlock(this->d_addr[i], buffer)) {
                return -1;
            }

            for (int j = 0; j < 128; ++j) {
                if (buffer[j] > 0) {
                    SuperBlock::superBlock.ReleaseBlock(buffer[j]);
                }
                else {
                    break;
                }
            }

            SuperBlock::superBlock.ReleaseBlock(this->d_addr[i]);
        }
    }

    int buffer2[128];
    for (int i = 8; i < 10; ++i) {
        if (this->d_addr[i] > 0) {
            // 二级索引有效
            if (BLOCK_SIZE != DeviceManager::deviceManager.ReadBlock(this->d_addr[i], buffer)) {
                return -1;
            }

            for (int j = 0; j < 128; ++j) {
                if (buffer[j] > 0) {
                    // 一级索引有效
                    if (BLOCK_SIZE != DeviceManager::deviceManager.ReadBlock(buffer[j], buffer2)) {
                        return -1;
                    }

                    for (int k = 0; k < 128; ++k) {
                        if (buffer2[k] > 0) {
                            SuperBlock::superBlock.ReleaseBlock(buffer2[k]);
                        }
                        else {
                            break;
                        }
                    }

                    SuperBlock::superBlock.ReleaseBlock(buffer[j]);
                }
                else {
                    break;
                }
            }

            SuperBlock::superBlock.ReleaseBlock(this->d_addr[i]);
        }
        else {
            break;
        }
    }
    return 0;
}

/**
 * @brief DiskInode析构函数
 */
DiskInode::~DiskInode() = default;
