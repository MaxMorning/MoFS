/**
 * @file DeviceManager.cpp
 * @brief 设备管理器实现，负责和映象文件进行I/O
 * @author 韩孟霖
 * @date 2022/5/4
 * @license GPL v3
 */
#include "../../utils/Diagnose.h"
#include "../../include/device/DeviceManager.h"
#include "../../include/SuperBlock.h"
#include "../../include/MoFSErrno.h"

/// 映象文件的默认偏移量
#define DEFAULT_OFFSET 256 * 1024

DeviceManager DeviceManager::deviceManager;

DeviceManager::DeviceManager() {
    this->imgFilePtr = nullptr;

    // 先给SuperBlock 和 inode区分配 256KB。这个值可以在SuperBlock加载时被修改。
    this->SetOffset(DEFAULT_OFFSET + HEADER_SIG_SIZE);
}

void DeviceManager::OpenImage(const char *imagePath) {
    // 尝试打开映象文件
    this->imgFilePtr = fopen(imagePath, "rb+");
    // 如果不存在，创建一个
    if (this->imgFilePtr == nullptr) {
        this->imgFilePtr = fopen(imagePath, "wb+");

        if (this->imgFilePtr == nullptr) {
            // 如果还是打不开，那肯定有问题
            MoFSErrno = 19;
            Diagnose::PrintError("Cannot open image : " + std::string(imagePath));
            return;
        }
    }
}

DeviceManager::~DeviceManager() {
    if (this->imgFilePtr != nullptr) {
        fclose(this->imgFilePtr);
    }
}

void DeviceManager::SetOffset(int offset) {
    this->blockContentOffset = offset;
}

unsigned int DeviceManager::ReadBlock(int blockNo, void *buffer) {
    int dstOffset = blockNo * BLOCK_SIZE + this->blockContentOffset;
    fseek(this->imgFilePtr, dstOffset, SEEK_SET);

    return fread(buffer, 1, BLOCK_SIZE, this->imgFilePtr);
}

unsigned int DeviceManager::WriteBlock(int blockNo, void *buffer, int size) {
    int dstOffset = blockNo * BLOCK_SIZE + this->blockContentOffset;
    fseek(this->imgFilePtr, dstOffset, SEEK_SET);

    return fwrite(buffer, 1, size, this->imgFilePtr);
}

int DeviceManager::ReadInode(int inodeNo, DiskInode *inodePtr) {
    unsigned int dstOffset = 64 + inodeNo * sizeof(DiskInode) + HEADER_SIG_SIZE;
    fseek(this->imgFilePtr, dstOffset, SEEK_SET);

    unsigned int readByte = fread(inodePtr,  1, sizeof(DiskInode),this->imgFilePtr);
    if (readByte != sizeof(DiskInode)) {
        MoFSErrno = 16;
        return -1;
    }

    return 0;
}

int DeviceManager::WriteInode(int inodeNo, DiskInode *inodePtr) {
    unsigned int dstOffset = 64 + inodeNo * sizeof(DiskInode) + HEADER_SIG_SIZE;
    fseek(this->imgFilePtr, dstOffset, SEEK_SET);

    unsigned int writeByte = fwrite(inodePtr, 1, sizeof(DiskInode), this->imgFilePtr);

    if (writeByte != sizeof(DiskInode)) {
        MoFSErrno = 16;
        return -1;
    }

    return 0;
}

int DeviceManager::LoadSuperBlock(void *superBlockPtr) {
    fseek(this->imgFilePtr, HEADER_SIG_SIZE, SEEK_SET);
    unsigned int readByte = fread(superBlockPtr, sizeof(SuperBlock), 1, this->imgFilePtr);

    if (readByte != 1) {
        MoFSErrno = 16;
        return -1;
    }

    SuperBlock* ptr = (SuperBlock*) superBlockPtr;
    this->blockContentOffset = ptr->s_isize * BLOCK_SIZE + sizeof(SuperBlock) + HEADER_SIG_SIZE;

    return 0;
}

int DeviceManager::StoreSuperBlock(void *superBlockPtr) {
    fseek(this->imgFilePtr, HEADER_SIG_SIZE, SEEK_SET);
    unsigned int writeByte = fwrite(superBlockPtr, sizeof(SuperBlock), 1, this->imgFilePtr);

    if (writeByte != 1) {
        MoFSErrno = 16;
        return -1;
    }

    SuperBlock* ptr = (SuperBlock*) superBlockPtr;
    this->blockContentOffset = ptr->s_isize * BLOCK_SIZE + sizeof(SuperBlock) + HEADER_SIG_SIZE;

    return 0;
}
