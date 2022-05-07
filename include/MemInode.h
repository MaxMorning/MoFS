/**
 * @file MemInode.h
 * @brief 内存inode头文件
 * @mainpage MemInode
 * @author 韩孟霖
 * @date 2022/05/03
 * @license GPL v3
 */

#ifndef MOFS_MEMINODE_H
#define MOFS_MEMINODE_H

#include "DiskInode.h"

#define SYSTEM_MEM_INODE_NUM 512

/**
 * @brief i_flag中标志位
 */
enum INodeFlag
{
    ILOCK = 0x1,		///< 索引节点上锁
    IUPD  = 0x2,		///< 内存inode被修改过，需要更新相应外存inode
    IACC  = 0x4,		///< 内存inode被访问过，需要修改最近一次访问时间
    IMOUNT = 0x8,		///< 内存inode用于挂载子文件系统
    IWANT = 0x10,		///< 有进程正在等待该内存inode被解锁，清ILOCK标志时，要唤醒这种进程
    ITEXT = 0x20		///< 内存inode对应进程图像的正文段
};

/**
 * @brief 内存索引结点(MemInode)
 */
class MemInode {
public:
    /* static const member */
    static const unsigned int IALLOC = 0x8000;		///< 文件被使用
    static const unsigned int IFMT = 0x6000;		///< 文件类型掩码
    static const unsigned int IFDIR = 0x4000;		///< 文件类型：目录文件
    static const unsigned int IFCHR = 0x2000;		///< 字符设备特殊类型文件
    static const unsigned int IFBLK = 0x6000;		///< 块设备特殊类型文件，为0表示常规数据文件
    static const unsigned int ILARG = 0x1000;		///< 文件长度类型：大型或巨型文件
    static const unsigned int ISUID = 0x800;		///< 执行时文件时将用户的有效用户ID修改为文件所有者的User ID
    static const unsigned int ISGID = 0x400;		///< 执行时文件时将用户的有效组ID修改为文件所有者的Group ID
    static const unsigned int ISVTX = 0x200;		///< 使用后仍然位于交换区上的正文段
    static const unsigned int IREAD = 0x100;		///< 对文件的读权限
    static const unsigned int IWRITE = 0x80;		///< 对文件的写权限
    static const unsigned int IEXEC = 0x40;			///< 对文件的执行权限
    static const unsigned int IRWXU = (IREAD|IWRITE|IEXEC);		///< 文件主对文件的读、写、执行权限
    static const unsigned int IRWXG = ((IRWXU) >> 3);			///< 文件主同组用户对文件的读、写、执行权限
    static const unsigned int IRWXO = ((IRWXU) >> 6);			///< 其他用户对文件的读、写、执行权限

    static const int SMALL_FILE_BLOCK = 6;	///< 小型文件：直接索引表最多可寻址的逻辑块号
    static const int LARGE_FILE_BLOCK = 128 * 2 + 6;	///< 大型文件：经一次间接索引表最多可寻址的逻辑块号
    static const int HUGE_FILE_BLOCK = 128 * 128 * 2 + 128 * 2 + 6;	///< 巨型文件：经二次间接索引最大可寻址文件逻辑块号

    /* Functions */
public:
    /**
     * @brief MemInode构造工厂，有可能会从磁盘中加载DiskInode
     * @param diskInodeIdx DiskInode序号
     * @param memInodePtr 结果MemInode，函数在此改写其指向的地址
     * @return 0表示成功，-1表示失败
     * @note 所有MemInode的实例化都在systemMemInodeTable中存储，不允许在其它地方实例化
     */
    static int MemInodeFactory(int diskInodeIdx, MemInode*& memInodePtr);

    /**
     * @brief 单纯地为一个新的MemInode分配一个entry，不做初始化，仅设置占用标记
     * @param memInodePtr 函数在此改写其指向的地址
     * @return 0表示成功，-1表示失败
     */
    static int MemInodeNotInit(MemInode*& memInodePtr);

    /* Destructors */
    ~MemInode() = default;

    /**
     * @brief 根据Inode对象中的物理磁盘块索引表，读取相应的文件数据
     * @param offset 起始偏移量
     * @param buffer 读取缓冲区
     * @param size 读取字节数
     * @return 返回实际读取字节数，-1表示错误
     */
    int Read(int offset, char* buffer, int size);

    /**
     * @brief 根据Inode对象中的物理磁盘块索引表，将数据写入文件
     * @param offset 起始偏移量
     * @param buffer 写入缓冲区
     * @param size 写入字节数
     * @return 返回实际读取字节数，-1表示错误
     */
    int Write(int offset, char* buffer, int size);

    /**
     * @brief 扩展文件大小
     * @param newSize 新的文件大小
     * @return 0表示成功，-1表示失败
     */
    int Expand(int newSize);

    /**
     * @brief 将文件的逻辑块号转换成对应的物理盘块号
     * @param logicBlockIndex 逻辑块号
     * @return 物理块号
     */
    int BlockMap(int logicBlockIndex);

    /**
     * @brief 释放占用的所有Block
     * @return 0表示成功，-1表示失败
     */
    int ReleaseBlocks();

    /**
     * @brief 关闭inode，写回磁盘
     * @param lastAccTime 最后访问时间
     * @param lastModTime 最后修改时间
     * @return 0表示成功，-1表示失败
     */
    int Close(int lastAccTime, int lastModTime);

    /* Members */
public:
    unsigned int i_flag;	///< 状态的标志位，定义见enum INodeFlag
    unsigned int i_mode;	///< 文件工作方式信息

    int		i_count;		///< 引用计数，指的是有多少OpenFile连接至此
    int		i_nlink;		///< 文件联结计数，即该文件在目录树中不同路径名的数量

    short	i_dev;			///< 外存inode所在存储设备的设备号
    int		i_number;		///< 外存inode区中的编号

    short	i_uid;			///< 文件所有者的用户标识数
    short	i_gid;			///< 文件所有者的组标识数

    int		i_size;			///< 文件大小，字节为单位
    int		i_addr[10];		///< 用于文件逻辑块好和物理块好转换的基本索引表

    int		i_used;		    ///< 指示该inode是否有效。在systemMemInodeTable中，若为1则表示有效，0表示空闲。
                            ///< 在UNIX V6++中，这里存放最近一次读取文件的逻辑块号，用于判断是否需要预读。

    static MemInode systemMemInodeTable[SYSTEM_MEM_INODE_NUM]; ///< 系统全局inode表

private:
    /**
     * 构造函数，不允许在其它地方实例化MemInode
     */
    MemInode() = default;
};
#endif //MOFS_MEMINODE_H
