/**
 * @file DiskInode.h
 * @brief 外存inode头文件
 * @mainpage DiskInode
 * @author 韩孟霖
 * @date 2022-05-03
 * @license GPL v3
 */

#ifndef MOFS_DISK_INODE_H
#define MOFS_DISK_INODE_H

/**
 * @brief 外存索引节点(DiskINode)
 *
 */
class DiskInode {
public:
    /* Functions */
    /**
     * @brief DiskInode构造函数
     */
    DiskInode();

    /**
     * @brief DiskInode析构函数
     */
    ~DiskInode();

    /* Members */
    unsigned int d_mode;    ///< 状态的标志位，定义见enum INodeFlag
    int d_nlink;            ///< 文件联结计数，即该文件在目录树中不同路径名的数量

    short d_uid;            ///< 文件所有者的用户标识数
    short d_gid;            ///< 文件所有者的用户组标识数

    int d_size;             ///< 文件大小，以字节为单位
    int d_addr[10];         ///< 用于文件逻辑块号和物理块号转换的基本索引表

    int d_atime;            ///< 最后访问时间
    int d_mtime;            ///< 最后修改时间
};

#endif //MOFS_DISK_INODE_H
