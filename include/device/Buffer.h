/**
 * @file Buffer.h
 * @brief 内存缓存头文件。MoFS的缓存机制是向上透明的，因此可以方便地切换不同的缓存机制
 * @author 韩孟霖
 * @date 2022/5/11
 * @license GPL v3
 */

#ifndef MOFS_BUFFER_H
#define MOFS_BUFFER_H

/**
 * 实现缓存机制的链表，可以适配block和inode的缓存机制
 */
class BufferLinkList {
public:
    /**
     * @brief 构造函数
     * @param bufferNum 缓存块数量
     */
    explicit BufferLinkList(int bufferNum);

    /**
     * @brief 析构函数
     */
    ~BufferLinkList();

    // 这里采用序号的方式而非使用指针相互勾结，主要考量是在搜寻需要的块时，连续的内存地址能够增加CPU L1 cache hit 的概率
    int* prevLinkList; ///< 指示每个结点的前一项是谁（尾结点 -> 头结点）
    int* nextLinkList; ///< 指示每个结点的后一项是谁 （头结点 -> 尾结点）
    int* numberLinkList; ///< 指示缓存的块编号

    int headPtr; ///< 指向头结点
    int rearPtr; ///< 指向尾结点（最后一个占用结点），指向-1表示结点全空闲

    /**
     * @brief 获取目标块对应的缓存块的地址
     * @param blockIdx 目标块序号
     * @return 缓存块序号，-1表示未缓存
     */
    int GetBufferedIndex(int blockIdx);

    /**
     * @brief 分配一个新的缓存块
     * @param blockIdx 目标块序号
     * @param oldBlockIdx 被换出的块序号，-1为未换出
     * @return 分配的缓存块序号，缓存机制保证一定返回有意义的值
     */
    int AllocNewBuffer(int blockIdx, int &oldBlockIdx);

    /**
     * 初始化链表
     * @param bufferNum 缓存块数量
     * @return
     */
    void InitLinkList(int bufferNum);
private:
    /**
     * @brief 将bufferIdx插入队首
     * @param bufferIdx 缓存块序号
     */
    void InsertHead(int bufferIdx);

    /**
     * @brief 打印链表内容，仅debug
     */
    void DebugPrintList();
};

#endif //MOFS_BUFFER_H
