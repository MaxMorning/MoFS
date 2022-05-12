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
    this->prevLinkList = new int[bufferNum];

    this->nextLinkList = new int[bufferNum];

    this->numberLinkList = new int[bufferNum];

    this->InitLinkList(bufferNum);
}

BufferLinkList::~BufferLinkList() {
    delete[] this->prevLinkList;
    delete[] this->nextLinkList;
    delete[] this->numberLinkList;
}

int BufferLinkList::GetBufferedIndex(int blockIdx) {
    int searchPtr = this->headPtr;
    while (searchPtr >= 0) {
        if (this->numberLinkList[searchPtr] == blockIdx) {
            // cache hit
            if (searchPtr != this->headPtr) {
                // 如果找到的结点是头结点，没有必要调用insertHead
                this->InsertHead(searchPtr);
            }
            return searchPtr;
        }

        searchPtr = this->nextLinkList[searchPtr];
    }

    return -1;
}

int BufferLinkList::AllocNewBuffer(int blockIdx, int &oldBlockIdx) {
    int allocIdx = -1;
    if (this->rearPtr == -1) {
        // 当前全空
        allocIdx = this->headPtr;
    }
    else {
        allocIdx = this->nextLinkList[this->rearPtr];
    }
    oldBlockIdx = -1;
    if (allocIdx == -1) {
        // 没有空闲缓存块，释放队尾的缓存块
        allocIdx = this->rearPtr;
        oldBlockIdx = this->numberLinkList[allocIdx];
    }

    // 将新分配的缓存块插入队首
    this->InsertHead(allocIdx);

    this->numberLinkList[allocIdx] = blockIdx;

    return allocIdx;
}

void BufferLinkList::InsertHead(int bufferIdx) {
    // 断开bufferIdx和链表的连接
    int next_idx = this->nextLinkList[bufferIdx];
    int prev_idx = this->prevLinkList[bufferIdx];
    // 设置尾结点，prev_idx == -1表示分配的空闲buffer是队首结点，此时尾指针需要设置在新分配的buffer
    if (prev_idx == -1) {
        this->rearPtr = bufferIdx;
    }
    else if (bufferIdx == this->rearPtr) {
        this->rearPtr = this->prevLinkList[this->rearPtr];
    }
    // bufferIdx前结点的后指针指向bufferIdx后结点
    if (prev_idx >= 0) {
        this->nextLinkList[prev_idx] = next_idx;
    }
    else {
        // 前结点是头结点
        this->headPtr = next_idx;
    }

    // bufferIdx后结点的前指针指向bufferIdx的前结点
    if (next_idx >= 0) {
        this->prevLinkList[next_idx] = prev_idx;
    }

    // 将bufferIdx插入头部
    int ori_head = this->headPtr;
    // 原头结点的前指针指向bufferIdx
    this->prevLinkList[ori_head] = bufferIdx;
    // bufferIdx的后指针指向原头结点
    this->nextLinkList[bufferIdx] = ori_head;
    // bufferIdx的前指针指向head
    this->prevLinkList[bufferIdx] = -1;
    // head指向bufferIdx
    this->headPtr = bufferIdx;
}

void BufferLinkList::InitLinkList(int bufferNum) {
    this->prevLinkList[0] = -1;
    for (int i = 1; i < bufferNum; ++i) {
        this->prevLinkList[i] = i - 1;
    }

    for (int i = 0; i < bufferNum - 1; ++i) {
        this->nextLinkList[i] = i + 1;
    }
    this->nextLinkList[bufferNum - 1] = -1;


    memset(this->numberLinkList, -1, bufferNum * sizeof(int));

    this->headPtr = 0;
    this->rearPtr = -1;
}

#include <cstdio>

void BufferLinkList::DebugPrintList() {
    printf("\n%d\n", this->headPtr);

    int searchPtr = this->headPtr;

    while (searchPtr >= 0) {
        printf("%d ", searchPtr);
        searchPtr = this->nextLinkList[searchPtr];
    }

    printf("\n");
}
