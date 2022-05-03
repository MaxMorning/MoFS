/**
 * @file SuperBlock.cpp
 * @brief 超级块实现
 * @author 韩孟霖
 * @date 2022/5/4
 * @license GPL v3
 */

#include <ctime>

#include "../include/device/DeviceManager.h"
#include "../include/SuperBlock.h"
#include "../include/DiskInode.h"

int SuperBlock::MakeFS(int totalDiskByte, int inodeNum) {
    SuperBlock& superBlockRef = SuperBlock::superBlock;


    int inodeSegSize = sizeof(DiskInode) * inodeNum;

    superBlockRef.s_isize = (inodeSegSize + BLOCK_SIZE - 1) / BLOCK_SIZE;


    int remainByte = totalDiskByte - inodeSegSize - sizeof(SuperBlock);
    if (remainByte <= BLOCK_SIZE) {
        return -1;
    }

    superBlockRef.s_fsize = (remainByte + BLOCK_SIZE - 1) / BLOCK_SIZE;


    superBlockRef.s_nfree = 100;
    superBlockRef.s_ninode = 100;
    for (int i = 0; i < 100; ++i) {
        superBlockRef.s_free[i] = i;
        superBlockRef.s_inode[i] = i;
    }

    superBlockRef.s_flock = 0;
    superBlockRef.s_ilock = 0;

    superBlockRef.s_fmod = 0;
    superBlockRef.s_ronly = 0;
    superBlockRef.s_time = time(nullptr);

    DeviceManager::deviceManager.StoreSuperBlock(&superBlockRef);

    return 0;
}
