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

/// 映象文件的默认偏移量
#define DEFAULT_OFFSET 256 * 1024

DeviceManager::DeviceManager() {
    this->imgFilePtr = nullptr;

    // 先给SuperBlock 和 inode区分配 256KB。这个值可以在SuperBlock加载时被修改。
    this->SetOffset(DEFAULT_OFFSET + HEADER_SIG_SIZE);
}

void DeviceManager::OpenImage(const std::string &imagePath) {
    this->imgFilePtr = fopen(imagePath.c_str(), "rw");
    if (this->imgFilePtr == nullptr) {
        Diagnose::PrintError("Cannot open image : " + imagePath);
        return;
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

unsigned int DeviceManager::WriteBlock(int blockNo, void *buffer) {
    int dstOffset = blockNo * BLOCK_SIZE + this->blockContentOffset;
    fseek(this->imgFilePtr, dstOffset, SEEK_SET);

    return fwrite(buffer, 1, BLOCK_SIZE, this->imgFilePtr);
}

int DeviceManager::ReadInode(int inodeNo, DiskInode *inodePtr) {
    unsigned int dstOffset = 64 + inodeNo * sizeof(DiskInode);
    fseek(this->imgFilePtr, dstOffset, SEEK_SET);

    unsigned int readByte = fread(inodePtr, sizeof(DiskInode), 1, this->imgFilePtr);
    if (readByte != sizeof(DiskInode)) {
        return -1;
    }

    return 0;
}

int DeviceManager::WriteInode(int inodeNo, DiskInode *inodePtr) {
    unsigned int dstOffset = 64 + inodeNo * sizeof(DiskInode);
    fseek(this->imgFilePtr, dstOffset, SEEK_SET);

    unsigned int writeByte = fwrite(inodePtr, sizeof(DiskInode), 1, this->imgFilePtr);

    if (writeByte != sizeof(DiskInode)) {
        return -1;
    }

    return 0;
}

int DeviceManager::LoadSuperBlock(void *superBlockPtr) {
    fseek(this->imgFilePtr, 0, SEEK_SET);
    unsigned int readByte = fread(superBlockPtr, sizeof(SuperBlock), 1, this->imgFilePtr);

    if (readByte != sizeof(SuperBlock)) {
        return -1;
    }

    SuperBlock* ptr = (SuperBlock*) superBlockPtr;
    this->blockContentOffset = ptr->s_isize * BLOCK_SIZE + sizeof(SuperBlock) + HEADER_SIG_SIZE;

    return 0;
}

int DeviceManager::StoreSuperBlock(void *superBlockPtr) {
    fseek(this->imgFilePtr, 0, SEEK_SET);
    unsigned int writeByte = fwrite(superBlockPtr, sizeof(SuperBlock), 1, this->imgFilePtr);

    if (writeByte != sizeof(SuperBlock)) {
        return -1;
    }

    SuperBlock* ptr = (SuperBlock*) superBlockPtr;
    this->blockContentOffset = ptr->s_isize * BLOCK_SIZE + sizeof(SuperBlock) + HEADER_SIG_SIZE;

    return 0;
}
