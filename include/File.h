/**
 * @file File.h
 * @brief 文件类，每个实例代表一个打开的文件
 * @author 韩孟霖
 * @date 2022/5/3
 * @license GPL v3
 */

#ifndef MOFS_FILE_H
#define MOFS_FILE_H

#include "MemInode.h"

/**
 * @brief 文件读写属性enum
 */
enum FileFlags
{
    FREAD = 0x1,			///< 读请求类型
    FWRITE = 0x2,			///< 写请求类型
    FPIPE = 0x4				///< 管道类型(不使用)
};

class File {
public:
    /**
     * @brief 构造函数
     */
    File();

    /**
     * @brief 析构函数
     */
    ~File();


    /* Member */
    unsigned int    f_flag;		    ///< 对打开文件的读、写操作要求
    int		        f_count;		///< 当前引用该文件控制块的进程数量
    MemInode*	    f_inode;		///< 指向打开文件的内存Inode指针
    int		        f_offset;		///< 文件读写位置指针
};
#endif //MOFS_FILE_H
