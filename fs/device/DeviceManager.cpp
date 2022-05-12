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

    // 将两个dirty表置为false
    memset(this->blockDirty, 0, BLOCK_BUFFER_NUM * sizeof(bool));
    memset(this->inodeDirty, 0, INODE_BUFFER_NUM * sizeof(bool));
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
    // 将所有缓存的脏块写回文件
    int currentPtr = blockBufferManager.headPtr;
    while (currentPtr >= 0) {
        if (this->blockDirty[currentPtr]) {
            // 脏块
            this->WriteBlockToFile(currentPtr, this->blockBufferManager.numberLinkList[currentPtr]);
        }

        currentPtr = blockBufferManager.nextLinkList[currentPtr];
    }


    if (this->imgFilePtr != nullptr) {
        fclose(this->imgFilePtr);
    }
}

void DeviceManager::SetOffset(int offset) {
    this->blockContentOffset = offset;
}

unsigned int DeviceManager::ReadBlock(int blockNo, void *buffer) {
// 检查缓存
    int bufferIdx = blockBufferManager.GetBufferedIndex(blockNo);
    if (bufferIdx != -1) {
        // 有缓存
        memcpy(buffer, blockBuffer[bufferIdx], BLOCK_SIZE);
//        Diagnose::PrintLog("ReadBlock (Buffered) " + std::to_string(blockNo) + ' ' + std::to_string(bufferIdx));
        return BLOCK_SIZE;
    }

    // 没缓存
    int dstOffset = blockNo * BLOCK_SIZE + this->blockContentOffset;
    fseek(this->imgFilePtr, dstOffset, SEEK_SET);

    int readByteCnt = fread(buffer, 1, BLOCK_SIZE, this->imgFilePtr);
    if (readByteCnt == BLOCK_SIZE) {
        // 只缓存读满的，不过不出意外都是读满的
        int swapBlockIdx = -1;
        int newBufferIdx = blockBufferManager.AllocNewBuffer(blockNo, swapBlockIdx);
        if (swapBlockIdx != -1 && blockDirty[newBufferIdx]) {
            // newBufferIdx指向的块的内容需要被写回磁盘中
            this->WriteBlockToFile(newBufferIdx, swapBlockIdx);
        }
        memcpy(blockBuffer[newBufferIdx], buffer, BLOCK_SIZE);
        this->blockDirty[newBufferIdx] = false;
//        Diagnose::PrintLog("ReadBlock (No buffered) " + std::to_string(blockNo) + ' ' + std::to_string(newBufferIdx) + " current rear : " + std::to_string(blockBufferManager.rearPtr));
    }
    return readByteCnt;
}

unsigned int DeviceManager::WriteBlock(int blockNo, void *buffer) {
    // 检查缓存
    int bufferIdx = blockBufferManager.GetBufferedIndex(blockNo);
    if (bufferIdx != -1) {
        // 有缓存
        memcpy(blockBuffer[bufferIdx], buffer, BLOCK_SIZE);
        blockDirty[bufferIdx] = true;
//        Diagnose::PrintLog("WriteBlock (Buffered) " + std::to_string(blockNo) + ' ' + std::to_string(bufferIdx));
        return BLOCK_SIZE;
    }

    // 没缓存
    int swapBlockIdx = -1;
    int newBufferIdx = blockBufferManager.AllocNewBuffer(blockNo, swapBlockIdx);
    if (swapBlockIdx != -1 && blockDirty[newBufferIdx]) {
        // 有块因为新的缓存块需求而被释放，且该块脏
        // 需要写回磁盘
        this->WriteBlockToFile(newBufferIdx, swapBlockIdx);
    }
    memcpy(blockBuffer[newBufferIdx], buffer, BLOCK_SIZE);
    blockDirty[newBufferIdx] = true;
//    Diagnose::PrintLog("WriteBlock (No buffered) " + std::to_string(blockNo) + ' ' + std::to_string(newBufferIdx) + " : swap out block : " +
//                               std::to_string(swapBlockIdx) + " current rear : " + std::to_string(blockBufferManager.rearPtr));
    return BLOCK_SIZE;
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

unsigned int DeviceManager::WriteBlockToFile(int bufferIdx, int blockIdx) {
//    Diagnose::PrintLog("WriteBlockToFile " + std::to_string(bufferIdx) + ' ' + std::to_string(blockIdx));
    int dstOffset = blockIdx * BLOCK_SIZE + this->blockContentOffset;
    fseek(this->imgFilePtr, dstOffset, SEEK_SET);

    return fwrite(this->blockBuffer[bufferIdx], BLOCK_SIZE, 1, this->imgFilePtr);
}

int DeviceManager::WriteInodeToFile(int bufferIdx) {
    int inodeNo = this->inodeBufferManager.numberLinkList[bufferIdx];
    unsigned int dstOffset = 64 + inodeNo * sizeof(DiskInode) + HEADER_SIG_SIZE;
    fseek(this->imgFilePtr, dstOffset, SEEK_SET);

    unsigned int writeByte = fwrite(&(this->inodeBuffer[bufferIdx]), 1, sizeof(DiskInode), this->imgFilePtr);

    if (writeByte != sizeof(DiskInode)) {
        MoFSErrno = 16;
        return -1;
    }

    return 0;
}
