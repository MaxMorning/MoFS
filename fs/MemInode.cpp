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
#include "../include/SuperBlock.h"

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

int min(int a, int b) {
    return a < b ? a : b;
}

int max(int a, int b) {
    return a > b ? a : b;
}

int MemInode::Read(int offset, char *buffer, int size) {
    int currentFileOffset = offset;
    int currentBufferOffset = 0;
    int actualReadDst = min(offset + size, this->i_size);

    int startLogicBlock = offset / BLOCK_SIZE;
    int endLogicBlock = (actualReadDst + BLOCK_SIZE - 1) / BLOCK_SIZE;

    char readBlockBuffer[BLOCK_SIZE];
    // 读取第一个逻辑块的数据
    unsigned int readByteCnt = DeviceManager::deviceManager.ReadBlock(this->BlockMap(startLogicBlock), readBlockBuffer);
    if (readByteCnt != BLOCK_SIZE) {
        return -1;
    }

    int expectedByteCnt = min(size, BLOCK_SIZE - offset % BLOCK_SIZE);
    memcpy(buffer, readBlockBuffer + (BLOCK_SIZE - expectedByteCnt), expectedByteCnt);
    currentFileOffset += expectedByteCnt;
    currentBufferOffset += expectedByteCnt;


    // 读取剩余的完整块，除了最后一块
    for (int i = startLogicBlock + 1; i < endLogicBlock; ++i) {
        readByteCnt = DeviceManager::deviceManager.ReadBlock(this->BlockMap(i), readBlockBuffer);

        if (readByteCnt != BLOCK_SIZE) {
            return -1;
        }

        memcpy(buffer + currentBufferOffset, readBlockBuffer, BLOCK_SIZE);

        currentFileOffset += BLOCK_SIZE;
        currentBufferOffset += BLOCK_SIZE;
    }


    // 读取最后一块，也有可能是一块完整的
    readByteCnt = DeviceManager::deviceManager.ReadBlock(this->BlockMap(endLogicBlock), readBlockBuffer);
    if (readByteCnt != BLOCK_SIZE) {
        return -1;
    }

    expectedByteCnt = (actualReadDst - 1) % BLOCK_SIZE + 1;
    memcpy(buffer + currentBufferOffset, readBlockBuffer, expectedByteCnt);

    currentFileOffset += expectedByteCnt;
    currentBufferOffset += expectedByteCnt;


    return currentBufferOffset;
}

int MemInode::Write(int offset, char *buffer, int size) {
    bool needExpand = offset + size > this->i_size;

    if (needExpand) {
        // 写入之后的大小大于目前文件大小，需要扩展
        if (this->Expand(offset + size) == -1) {
            return -1;
        }
    }

    int currentFileOffset = offset;
    int currentBufferOffset = 0;

    int startLogicBlock = offset / BLOCK_SIZE;
    int endLogicBlock = (this->i_size + BLOCK_SIZE - 1) / BLOCK_SIZE;

    char writeBlockBuffer[BLOCK_SIZE];

    // 写第一个block的内容
    if (offset % BLOCK_SIZE == 0) {
        // 无需加载块，直接写入即可
        unsigned int writeByteCnt = DeviceManager::deviceManager.WriteBlock(this->BlockMap(startLogicBlock), buffer);

        if (writeByteCnt != BLOCK_SIZE) {
            return -1;
        }
    }
    else {
        // 需要先加载，修改后再写入
        unsigned int readByteCnt = DeviceManager::deviceManager.ReadBlock(this->BlockMap(startLogicBlock), writeBlockBuffer);

        if (readByteCnt != BLOCK_SIZE) {
            return -1;
        }

        int expectedByteCnt = min(size, BLOCK_SIZE - offset % BLOCK_SIZE);
        memcpy(writeBlockBuffer + (offset % BLOCK_SIZE), buffer, expectedByteCnt);

        currentBufferOffset += expectedByteCnt;
        currentFileOffset += expectedByteCnt;

        unsigned int writeByteCnt = DeviceManager::deviceManager.WriteBlock(this->BlockMap(startLogicBlock), writeBlockBuffer);
        if (writeByteCnt != BLOCK_SIZE) {
            return -1;
        }
    }

    // 写剩余完整块，除了最后一块
    for (int i = startLogicBlock + 1; i <= endLogicBlock; ++i) {
        unsigned int writeByteCnt = DeviceManager::deviceManager.WriteBlock(this->BlockMap(i), buffer + currentBufferOffset);

        if (writeByteCnt != BLOCK_SIZE) {
            return -1;
        }

        currentFileOffset += BLOCK_SIZE;
        currentBufferOffset += BLOCK_SIZE;
    }

    // 写最后一块
    if (offset + size < this->i_size) {
        // 文件尾部仍然有一些内容没有被修改，需要加载最后一块
        unsigned int readByteCnt = DeviceManager::deviceManager.ReadBlock(this->BlockMap(endLogicBlock), writeBlockBuffer);
        if (readByteCnt != BLOCK_SIZE) {
            return -1;
        }
    }

    int expectedByteCnt = (offset + size - 1) % BLOCK_SIZE + 1;
    memcpy(writeBlockBuffer, buffer + currentBufferOffset, expectedByteCnt);

    unsigned int writeByteCnt = DeviceManager::deviceManager.WriteBlock(this->BlockMap(endLogicBlock), writeBlockBuffer);

    if (writeByteCnt != BLOCK_SIZE) {
        return -1;
    }

    currentFileOffset += expectedByteCnt;
    currentBufferOffset += expectedByteCnt;

    return currentBufferOffset;
}

int MemInode::Expand(int newSize) {
    int oldBlockNum = (this->i_size - 1) / BLOCK_SIZE + 1;
    int newBlockNum = (newSize - 1) / BLOCK_SIZE + 1;

    if (newBlockNum > 6 + 2 * 128 + 2 * 128 * 128) {
        return -1;
    }

    int indexBlockBuffer[128];

    if (newBlockNum == oldBlockNum) {
        // 尽管有所扩张，但占用的块数不变
        return 0;
    }

    int oldStage0 = max(0, min(6, oldBlockNum));
    int newStage0 = max(0, min(6, newBlockNum));

    int oldStage1 = max(0, min(2 * 128, oldBlockNum - 6));
    int newStage1 = max(0, min(2 * 128, newBlockNum - 6));

    int oldStage2 = max(0, min(2 * 128 * 128, oldBlockNum - 6 - 2 * 128));
    int newStage2 = max(0, min(2 * 128 * 128, newBlockNum - 6 - 2 * 128));

    if (oldStage0 < newStage0) {
        for (int i = oldStage0 + 1; i <= newStage0; ++i) {
            int freeBlock = SuperBlock::superBlock.AllocBlock();
            if (freeBlock == -1) {
                return -1;
            }

            this->i_addr[i - 1] = freeBlock;
        }
    }

    if (oldStage1 < newStage1) {
        // 第一张一级索引表
        if (this->i_addr[6] <= 0) {
            // 原来的文件没有一级索引
            this->i_addr[6] = SuperBlock::superBlock.AllocBlock();
            if (this->i_addr[6] == -1) {
                return -1;
            }
            memset(indexBlockBuffer, -1, BLOCK_SIZE);
        }
        else {
            // 从磁盘加载索引
            unsigned int readByteCnt = DeviceManager::deviceManager.ReadBlock(this->i_addr[6], indexBlockBuffer);
            if (readByteCnt == -1) {
                return -1;
            }
        }

        int stage1table1 = min(128, newStage1);
        for (int i = min(128, oldStage1); i < stage1table1; ++i) {
            indexBlockBuffer[i] = SuperBlock::superBlock.AllocBlock();
            if (indexBlockBuffer[i] == -1) {
                return -1;
            }
        }

        if (BLOCK_SIZE != DeviceManager::deviceManager.WriteBlock(this->i_addr[6], indexBlockBuffer)) {
            return -1;
        }


        // 第二张一级索引表
        if (newStage1 >= 128) {
            int stage1table2 = min(newStage1 - 128, 128);
            if (this->i_addr[7] <= 0) {
                // 原来的文件没有一级索引
                this->i_addr[6] = SuperBlock::superBlock.AllocBlock();
                if (this->i_addr[6] == -1) {
                    return -1;
                }
                memset(indexBlockBuffer, -1, BLOCK_SIZE);
            }
            else {
                // 从磁盘加载索引
                unsigned int readByteCnt = DeviceManager::deviceManager.ReadBlock(this->i_addr[7], indexBlockBuffer);
                if (readByteCnt == -1) {
                    return -1;
                }
            }

            for (int i = min(max(0, oldStage1 - 128), 128); i < stage1table2; ++i) {
                indexBlockBuffer[i] = SuperBlock::superBlock.AllocBlock();
                if (indexBlockBuffer[i] == -1) {
                    return -1;
                }
            }

            if (BLOCK_SIZE != DeviceManager::deviceManager.WriteBlock(this->i_addr[7], indexBlockBuffer)) {
                return -1;
            }
        }
    }


    if (oldStage2 < newStage2) {
        int index2Buffer[128];
        int temp = oldStage2;
        int loopIdx2 = temp % 128;
        temp /= 128;
        int loopIdx1 = temp % 128;
        int loopIdx0 = temp / 128;

        int idx = oldStage2;

        while (loopIdx0 < 2) {
            if (this->i_addr[loopIdx0 + 8] <= 0) {
                // 原来的文件没有二级索引
                this->i_addr[loopIdx0 + 8] = SuperBlock::superBlock.AllocBlock();
                if (this->i_addr[loopIdx0 + 8] == -1) {
                    return -1;
                }
                memset(indexBlockBuffer, -1, BLOCK_SIZE);
            }
            else {
                // 从磁盘加载索引
                unsigned int readByteCnt = DeviceManager::deviceManager.ReadBlock(this->i_addr[loopIdx0 + 8], indexBlockBuffer);
                if (readByteCnt == -1) {
                    return -1;
                }
            }


            while (loopIdx1 < 128) {
                if (indexBlockBuffer[loopIdx1] <= 0) {
                    // 原来的文件没有一级索引
                    indexBlockBuffer[loopIdx1] = SuperBlock::superBlock.AllocBlock();
                    if (indexBlockBuffer[loopIdx1] == -1) {
                        return -1;
                    }
                    memset(index2Buffer, -1, BLOCK_SIZE);
                }
                else {
                    // 从磁盘加载索引
                    unsigned int readByteCnt = DeviceManager::deviceManager.ReadBlock(indexBlockBuffer[loopIdx1], index2Buffer);
                    if (readByteCnt == -1) {
                        return -1;
                    }
                }


                while (loopIdx2 < 128) {
                    index2Buffer[loopIdx2] = SuperBlock::superBlock.AllocBlock();
                    if (index2Buffer[loopIdx2] == -1) {
                        return -1;
                    }

                    ++idx;
                    ++loopIdx2;

                    if (idx >= newStage2) {
                        break;
                    }
                }

                // 存储刚刚填好的index2Buffer
                if (BLOCK_SIZE != DeviceManager::deviceManager.WriteBlock(indexBlockBuffer[loopIdx1], index2Buffer)) {
                    return -1;
                }

                loopIdx2 = 0;
                ++loopIdx1;

                if (idx >= newStage2) {
                    break;
                }
            }

            // 存储刚刚填好的index2Buffer
            if (BLOCK_SIZE != DeviceManager::deviceManager.WriteBlock(this->i_addr[8 + loopIdx0], indexBlockBuffer)) {
                return -1;
            }

            loopIdx1 = 0;
            ++loopIdx0;

            if (idx >= newStage2) {
                break;
            }
        }
    }

    this->i_size = newSize;
    return 0;
}

int MemInode::ReleaseBlocks() {
    int blockNum = (this->i_size - 1) / BLOCK_SIZE + 1;

    for (int i = 0; i < 6; ++i) {
        if (this->i_addr[i] > 0) {
            SuperBlock::superBlock.ReleaseBlock(this->i_addr[i]);
        }
    }

    int buffer[128];
    for (int i = 6; i < 8; ++i) {
        if (this->i_addr[i] > 0) {
            // 一级索引有效
            if (BLOCK_SIZE != DeviceManager::deviceManager.ReadBlock(this->i_addr[i], buffer)) {
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

            SuperBlock::superBlock.ReleaseBlock(this->i_addr[i]);
        }
    }

    int buffer2[128];
    for (int i = 8; i < 10; ++i) {
        if (this->i_addr[i] > 0) {
            // 二级索引有效
            if (BLOCK_SIZE != DeviceManager::deviceManager.ReadBlock(this->i_addr[i], buffer)) {
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

            SuperBlock::superBlock.ReleaseBlock(this->i_addr[i]);
        }
        else {
            break;
        }
    }
    return 0;
}
