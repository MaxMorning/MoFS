/**
 * @file Buffer.cpp
 * @brief 内存缓存实现
 * @author 韩孟霖
 * @date 2022/5/11
 * @license GPL v3
 */
#include <cstring>

#include "../../include/device/Buffer.h"

BufferLinkList::BufferLinkList(int bufferNum) {
    this->forwardLinkList = new int[bufferNum];
    for (int i = 0; i < bufferNum - 1; ++i) {
        this->forwardLinkList[i] = i + 1;
    }
    this->forwardLinkList[bufferNum - 1] = -1;


    this->backwardLinkList = new int[bufferNum];
    this->backwardLinkList[0] = -1;
    for (int i = 1; i < bufferNum; ++i) {
        this->backwardLinkList[i] = i - 1;
    }



    this->numberLinkList = new int[bufferNum];
    memset(this->numberLinkList, -1, bufferNum * sizeof(int));

    this->headPtr = 0;
    this->rearPtr = 0;
}

BufferLinkList::~BufferLinkList() {
    delete[] this->forwardLinkList;
    delete[] this->backwardLinkList;
    delete[] this->numberLinkList;
}

int BufferLinkList::GetBufferedIndex(int blockIdx) {
    int searchPtr = this->headPtr;
    while (searchPtr >= 0) {
        if (this->numberLinkList[searchPtr] == blockIdx) {
            // cache hit
            this->InsertHead(searchPtr);
            return searchPtr;
        }

        searchPtr = this->backwardLinkList[searchPtr];
    }

    return -1;
}

int BufferLinkList::AllocNewBuffer(int blockIdx, bool &releaseBuffer) {
    int allocIdx = this->rearPtr;
    releaseBuffer = false;
    if (allocIdx == -1) {
        // 没有空闲缓存块，释放队尾的缓存块
        allocIdx = this->forwardLinkList[allocIdx];
        releaseBuffer = true;
    }

    // 将新分配的缓存块插入队首
    this->InsertHead(allocIdx);


    return allocIdx;
}

void BufferLinkList::InsertHead(int bufferIdx) {
    int old_head = this->headPtr;
    int new_rear = this->forwardLinkList[bufferIdx];

    // 原队首的前指针指向新块
    this->forwardLinkList[old_head] = bufferIdx;
    // 新队尾的后指针指向新块原来的后指针
    this->backwardLinkList[new_rear] = this->backwardLinkList[bufferIdx];
    // 尾指针指向新块原来的后指针
    this->rearPtr = this->backwardLinkList[bufferIdx];
    // 新块的前指针指向头结点
    this->forwardLinkList[bufferIdx] = -1;
    // 新块的后指针指向原队首
    this->backwardLinkList[bufferIdx] = old_head;
    // 头结点的后指针指向新块
    this->headPtr = bufferIdx;
    // 新块原来的后指针（即现在的rearPtr）的前指针指向新队尾
    if (this->rearPtr != -1) {
        this->forwardLinkList[this->rearPtr] = new_rear;
    }
}
