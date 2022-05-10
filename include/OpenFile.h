/**
 * @file OpenFile.h
 * @brief 文件类，每个实例代表一个打开的文件
 * @author 韩孟霖
 * @date 2022/5/3
 * @license GPL v3
 */

#ifndef MOFS_OPENFILE_H
#define MOFS_OPENFILE_H

#include "MemInode.h"

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2


/**
 * @brief 文件读写属性enum
 */
enum FileFlags
{
    FREAD = 0x1,			///< 读请求类型
    FWRITE = 0x2,			///< 写请求类型
    FPIPE = 0x4				///< 管道类型(不使用)
};

class OpenFile {
public:
    /**
     * @brief OpenFile工厂，构建并打开OpenFile结构，但不分配fd
     * @param openFile 返回openFile
     * @param diskInodeIdx DiskInode序号
     * @param uid user id
     * @param gid group id
     * @param flags 读写标记
     * @return 0表示成功，-1表示失败
     */
    static int OpenFileFactory(OpenFile& openFile, int diskInodeIdx, int uid, int gid, int flags);


    /**
     * @brief 读取相应的文件数据
     * @param buffer 读取缓冲区
     * @param size 读取字节数
     * @return 返回实际读取字节数，-1表示错误
     */
    int Read(char* buffer, int size);

    /**
     * @brief 将数据写入文件
     * @param buffer 写入缓冲区
     * @param size 写入字节数
     * @return 返回实际读取字节数，-1表示错误
     */
    int Write(char* buffer, int size);

    /**
     * @brief 关闭文件，但不一定释放inode
     * @param updateTime 是否更新时间
     * @return 0表示成功，-1表示失败
     */
    int Close(bool updateTime);

    /**
     * @brief 打开并检查权限
     * @param flag 打开方式
     * @param inode 指向对应的MemInode
     * @param uid user id
     * @param gid group id
     * @return 0表示成功，-1表示失败
     */
    int Open(int flag, MemInode* inode, int uid, int gid);

    /**
     * @brief 设置读写指针位置
     * @param offset 偏移量，可以为负
     * @param fromWhere 基准位置
     * @return 0表示成功，-1表示失败
     */
    int Seek(int offset, int fromWhere);


    /**
     * @brief 检查inode是否能满足flag
     * @param flag 打开方式
     * @param uid user id
     * @param gid group id
     * @return true为权限验证通过，false为不通过
     */
    bool CheckFlags(int flag, int uid, int gid);

    /**
     * @brief 判断是否为目录文件
     * @return
     */
    bool IsDirFile();

    /**
     * @brief 判断该目录文件是否有内容
     * @return
     */
    bool HaveFilesInDir();

    /* Member */
    unsigned int    f_flag;		    ///< 对打开文件的读、写操作要求
    int		        f_count;		///< 当前引用该文件控制块的进程数量
    MemInode*	    f_inode;		///< 指向打开文件的内存Inode指针
    int		        f_offset;		///< 文件读写位置指针
};
#endif //MOFS_OPENFILE_H
