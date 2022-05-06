/**
 * @file DeviceManager.h
 * @brief 块设备管理器，包含缓存机制
 * @author 韩孟霖
 * @date 2022/5/3
 * @license GPL v3
 */

#ifndef MOFS_DEVICEMANAGER_H
#define MOFS_DEVICEMANAGER_H
#include <string>
#include <cstdlib>

#include "../DiskInode.h"


/// 块大小(字节)
#define BLOCK_SIZE 512

/// 设备前部占用空间(字节)，包括引导区、内核等
#define HEADER_SIG_SIZE 200 * BLOCK_SIZE

/**
 * @brief 块设备管理器，包含缓存机制
 */
class DeviceManager {
public:
    /**
     * @brief 构造函数
     *
     */
    DeviceManager();

    /**
     * @brief 打开映象文件
     * @param imagePath 映象路径
     */
    void OpenImage(const std::string& imagePath);

    /**
     * @brief 加载SuperBlock
     * @param superBlockPtr 超级块在内存的地址
     * @return 0表示成功，-1表示出错
     * @note superBlockPtr在此处为void*是为了避免引入SuperBlock头文件
     */
     int LoadSuperBlock(void* superBlockPtr);

     /**
      * @brief 保存SuperBlock
      * @param superBlockPtr 超级块在内存的地址
      * @return 0表示成功，-1表示出错
      */
     int StoreSuperBlock(void* superBlockPtr);

    /**
     * @brief 析构函数
     */
    ~DeviceManager();

    static DeviceManager deviceManager; ///< DeviceManager单例

    /**
     * @brief 根据提供的blockNo，从磁盘或内存缓存中读取数据
     * @param blockNo 待读取的块号
     * @param buffer 待写入数据的缓冲区，调用者需要保证其有足够的空间
     * @return 返回实际读取的字节数
     */
    unsigned int ReadBlock(int blockNo, void *buffer);

    /**
     * @brief 根据提供的blockNo，向磁盘或内存缓存中写入数据
     * @param blockNo 待写入的块号
     * @param buffer 待写入块中的缓冲区数据
     * @param size 写入字节数，默认为BLOCK_SIZE
     * @return 返回实际写入的字节数
     */
    unsigned int WriteBlock(int blockNo, void *buffer, int size = BLOCK_SIZE);

    /**
     * @brief 读取指定编号的diskInode
     * @param inodeNo 编号
     * @param inodePtr DiskInode指针，指向一块可用的空间
     * @return 0表示成功，-1表示出错
     */
    int ReadInode(int inodeNo, DiskInode* inodePtr);

    /**
     * @brief 写入指定编号的diskInode
     * @param inodeNo 编号
     * @param inodePtr DiskInode指针，指向需要写入的inode
     * @return 0表示成功，-1表示出错
     */
    int WriteInode(int inodeNo, DiskInode* inodePtr);

    /**
     * @brief 设置block区起始的偏移量
     * @param offset 偏移量
     */
    void SetOffset(int offset);

private:
    FILE* imgFilePtr{}; ///< DeviceManager 持有的file指针
    int blockContentOffset{}; ///< block #0 从这个偏移量开始
};

#endif //MOFS_DEVICEMANAGER_H
