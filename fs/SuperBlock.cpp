/**
 * @file SuperBlock.cpp
 * @brief 超级块实现
 * @author 韩孟霖
 * @date 2022/5/4
 * @license GPL v3
 */

#include <ctime>

#include "../include/MoFSErrno.h"
#include "../include/device/DeviceManager.h"
#include "../include/SuperBlock.h"
#include "../utils/Diagnose.h"
#include "../include/MemInode.h"

SuperBlock SuperBlock::superBlock;

int SuperBlock::MakeFS(int totalDiskByte, int inodeNum) {
    SuperBlock& superBlockRef = SuperBlock::superBlock;


    int inodeSegSize = sizeof(DiskInode) * inodeNum;

    superBlockRef.s_isize = (inodeSegSize + BLOCK_SIZE - 1) / BLOCK_SIZE;


    int remainByte = totalDiskByte - inodeSegSize - sizeof(SuperBlock) - HEADER_SIG_SIZE;
    if (remainByte <= BLOCK_SIZE) {
        return -1;
    }

    int blockNum = (remainByte + BLOCK_SIZE - 1) / BLOCK_SIZE;
    superBlockRef.s_fsize = blockNum;

    superBlockRef.s_flock = 0;
    superBlockRef.s_ilock = 0;

    superBlockRef.s_fmod = 0;
    superBlockRef.s_ronly = 0;
    superBlockRef.s_time = time(nullptr);

    DeviceManager::deviceManager.SetOffset(HEADER_SIG_SIZE + superBlockRef.s_isize * BLOCK_SIZE + sizeof(SuperBlock));

    // 设置空闲块
    // 设置直接管辖的空闲块
    superBlockRef.s_nfree = (blockNum - 1) % 100 + 1;
    int directOffset = blockNum - superBlockRef.s_nfree;
    for (int i = 0; i < superBlockRef.s_nfree; ++i) {
        superBlockRef.s_free[i] = directOffset + i;
    }

    // 写入间接管辖的空闲块
    int freeBlocks[128]{};
    int hundreds = (blockNum - superBlockRef.s_nfree) / 100;
    freeBlocks[0] = 100;
    for (int i = 0; i < hundreds; ++i) {
        for (int j = 0; j < 100; ++j) {
            freeBlocks[j + 1] = i * 100 + j;
        }

        DeviceManager::deviceManager.WriteBlock((i + 1) * 100, freeBlocks);
    }


    // 设置空闲inode
    // 设置直接管辖的inode
    superBlockRef.s_ninode = (inodeNum - 1) % 100 + 1;
    directOffset = inodeNum - superBlockRef.s_ninode;
    for (int i = 0; i < superBlockRef.s_ninode; ++i) {
        superBlockRef.s_inode[i] = directOffset + i;
    }

    // 写入间接管辖的inode
    // 复用freeBlocks
    hundreds = (inodeNum - superBlockRef.s_ninode) / 100;
    freeBlocks[0] = 0;
    for (int i = 0; i < hundreds; ++i) {
        for (int j = 0; j < 100; ++j) {
            freeBlocks[j + 1] = i * 100 + j;
        }

        int freeBlock = SuperBlock::superBlock.AllocBlock();
        if (freeBlock == -1) {
            Diagnose::PrintError("Cannot alloc free block.");
            return -1;
        }
        DeviceManager::deviceManager.WriteBlock(freeBlock, freeBlocks);
        freeBlocks[0] = freeBlock;
    }

    SuperBlock::superBlock.s_nextInodeBlk = freeBlocks[0];


    // 0号块不使用
    // 创建根目录文件
    SuperBlock::superBlock.s_rootInode = SuperBlock::superBlock.AllocDiskInode();
    if (SuperBlock::superBlock.s_rootInode == -1) {
        Diagnose::PrintError("Cannot alloc free disk inode.");

        return -1;
    }

    DiskInode rootInode;
    rootInode.d_mode = MemInode::IALLOC | MemInode::IFDIR | 0777;

    rootInode.d_nlink = 1;
    rootInode.d_uid = 0;
    rootInode.d_gid = 0;
    rootInode.d_size = 0; /// @note 这里大小设置为0可能会出错
    memset(rootInode.d_addr, 0, 10 * sizeof(int));
    rootInode.d_atime = std::time(nullptr);
    rootInode.d_mtime = rootInode.d_atime;
    // 将根目录文件写入0号块
    DeviceManager::deviceManager.WriteInode(SuperBlock::superBlock.s_rootInode, &rootInode);

    // superBlock 写回磁盘
    DeviceManager::deviceManager.StoreSuperBlock(&superBlockRef);
    return 0;
}

int SuperBlock::AllocBlock() {
    if (this->s_nfree <= 0) {
        MoFSErrno = 11;
        return -1;
    }

    int freeBlock = this->s_free[this->s_nfree - 1]; // 取出最后一个直接管辖的空闲块
    if (this->s_nfree == 1) {
        // 分配完这个块之前，没有空闲块被superBlock直接管辖，需要将s_free[0]指向的块的内容调入
        // 调入后，s_free[0]指向的块被释放，可以作为空闲块返回
        if (this->s_free[0] == 0) {
            // 已经没有空闲块了
            MoFSErrno = 11;
            return -1;
        }

        int blockContent[BLOCK_SIZE / sizeof(int)];
        if (BLOCK_SIZE != DeviceManager::deviceManager.ReadBlock(this->s_free[0], blockContent)) {
            MoFSErrno = 16;
            return -1;
        }

        // 这里的memcpy将同时写s_nfree 和 s_free
        memcpy(&(this->s_nfree), blockContent, 101 * sizeof(int));

        return freeBlock;
    }
    else {
        // 当前superBlock直接管理一块或更多空闲块
        --(this->s_nfree);
        this->s_free[this->s_nfree] = 0;
        return freeBlock;
    }
}

int SuperBlock::ReleaseBlock(int blockIdx) {
    if (this->s_nfree == 100) {
        // 当前superBlock直接管辖的空闲块已满，将当前的这101字写入一个块中。这里存入blockIdx这个待释放的块中。
        int readBuffer[128];
        DeviceManager::deviceManager.WriteBlock(blockIdx, readBuffer);
        memcpy(&(this->s_nfree), readBuffer, 101 * sizeof(int));

        this->s_nfree = 1;
        this->s_free[0] = blockIdx;
    }
    else if (this->s_nfree == 0) {
        // 当前设备没有任何空闲块
        // this->s_free[0] 应为 0
        this->s_free[1] = blockIdx;
        this->s_nfree = 2;
    }
    else {
        this->s_free[this->s_nfree] = blockIdx;
        ++(this->s_nfree);
    }
    return 0;
}

int SuperBlock::AllocDiskInode() {
    if (this->s_ninode <= 0) {
        // 当前没有可分配的inode
        MoFSErrno = 15;
        return -1;
    }

    --(this->s_ninode);
    int freeInode = this->s_inode[this->s_ninode];
    this->s_inode[this->s_ninode] = 0;

    if (this->s_ninode == 0) {
        // 直接管辖的inode已经耗尽，需要重新从加载一批
        if (this->s_nextInodeBlk != 0) {
            // 仍然有block存着空闲inode
            int buffer[BLOCK_SIZE / sizeof(int)];
            DeviceManager::deviceManager.ReadBlock(this->s_nextInodeBlk, buffer);

            this->s_nextInodeBlk = buffer[0];
            memcpy(this->s_inode, &(buffer[1]), 100 * sizeof(int));
        }
    }

    return freeInode;
}

int SuperBlock::ReleaseInode(int inodeIdx) {
    if (this->s_ninode == 100) {
        // 当前直接管辖的inode已达上限，找一个block写入
        int blockIdx = this->AllocBlock();
        if (blockIdx == -1) {
            Diagnose::PrintError("Cannot alloc free block.");
            return -1;
        }

        int buffer[128]{};
        buffer[0] = blockIdx;
        memcpy(&(buffer[1]), this->s_inode, 100 * sizeof(int));

        DeviceManager::deviceManager.WriteBlock(blockIdx, buffer);

        this->s_ninode = 1;
        this->s_inode[0] = inodeIdx;
        this->s_nextInodeBlk = blockIdx;
        return 0;
    }
    else {
        this->s_inode[this->s_ninode] = inodeIdx;
        ++(this->s_ninode);
        return 0;
    }
}
