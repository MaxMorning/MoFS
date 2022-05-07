/**
 * @file SuperBlock.h
 * @brief 超级块
 * @author 韩孟霖
 * @date 2022/5/3
 * @license GPL v3
 */

#ifndef MOFS_SUPERBLOCK_H
#define MOFS_SUPERBLOCK_H

/**
 * @brief 超级块类
 */
class SuperBlock {
    /* Functions */
public:
    /**
     * @brief 构造函数
     */
    SuperBlock() = default;

    /**
     * @brief 析构函数
     */
    ~SuperBlock() = default;

    /**
     * 格式化
     * @param totalDiskByte 待格式化的磁盘字节数，会创建这么大的磁盘映象
     * @param inodeNum 最大inode数量
     * @return 0表示成功，-1表示出错
     */
    static int MakeFS(int totalDiskByte, int inodeNum);

    /**
     * @brief 分配一个块，存数据
     * @return 分配到的块号，-1表示出错
     */
    int AllocBlock();

    /**
     * @brief 释放一个块，标记为空闲块
     * @param blockIdx 待释放的块号
     * @return 0表示成功，-1表示出错
     */
    int ReleaseBlock(int blockIdx);

    /**
     * @brief 分配一个DiskInode
     * @return 分配到的DiskInode号，-1表示出错
     */
    int AllocDiskInode();

    /**
     * @brief 释放一个Inode
     * @param inodeIdx 待释放的Inode
     * @return 0表示成功，-1表示出错
     */
    int ReleaseInode(int inodeIdx);

    /* Members */
public:
    int		s_isize;		///< 外存Inode区占用的盘块数
    int		s_fsize;		///< 盘块总数

    int		s_nfree;		///< 直接管理的空闲盘块数量
    int		s_free[100];	///< 直接管理的空闲盘块索引表

    int		s_ninode;		///< 直接管理的空闲外存Inode数量
    int     s_nextInodeBlk; ///< 指向存储空闲Inode的下一个Block
    int		s_inode[100];	///< 直接管理的空闲外存Inode索引表
    int     s_rootInode;    ///< 根目录inode编号

    int		s_flock;		///< 封锁空闲盘块索引表标志
    int		s_ilock;		///< 封锁空闲Inode表标志

    int		s_fmod;			///< 内存中super block副本被修改标志，意味着需要更新外存对应的Super Block
    int		s_ronly;		///< 本文件系统只能读出
    int		s_time;			///< 最近一次更新时间
    int		padding[45];	///< 填充使SuperBlock块大小等于1024字节，占据2个扇区


    static SuperBlock superBlock; ///< SuperBlock单例
};

#endif //MOFS_SUPERBLOCK_H
