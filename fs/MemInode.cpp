/**
 * @file MemInode.cpp
 * @brief 内存Inode实现
 * @author 韩孟霖
 * @date 2022/5/3
 * @license GPL v3
 */
#include <cstring>
#include <ctime>

#include "../include/device/DeviceManager.h"
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

int MemInode::BlockMap(int logicBlockIndex) {
    if (logicBlockIndex < 6) {
        // 小型文件
        return this->i_addr[logicBlockIndex];
    }
    else if (logicBlockIndex < 6 + 2 * 128) {
        // 大型文件
        if (logicBlockIndex < 6 + 1 * 128) {
            int index = this->i_addr[6];
            int indices[128];
            DeviceManager::deviceManager.ReadBlock(index, indices);

            logicBlockIndex -= 6;
            return indices[logicBlockIndex];
        }
        else {
            int index = this->i_addr[7];
            int indices[128];
            DeviceManager::deviceManager.ReadBlock(index, indices);

            logicBlockIndex -= 6 + 1 * 128;
            return indices[logicBlockIndex];
        }
    }
    else {
        // 巨型文件
        logicBlockIndex -= 6 + 2 * 128;
        int level1index = logicBlockIndex / (128 * 128);
        logicBlockIndex = logicBlockIndex % (128 * 128);

        int level2index = logicBlockIndex / 128;
        int level3index = logicBlockIndex % 128;

        int indices[128];
        DeviceManager::deviceManager.ReadBlock(this->i_addr[8 + level1index], indices);

        DeviceManager::deviceManager.ReadBlock(indices[level2index], indices);

        return indices[level3index];
    }
}
