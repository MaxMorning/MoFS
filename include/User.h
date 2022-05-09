/**
 * @file User.h
 * @brief 
 * @author 韩孟霖
 * @date 2022/5/6
 * @license GPL v3
 */

#ifndef MOFS_USER_H
#define MOFS_USER_H

#include <string>

#include "SuperBlock.h"
#include "MemInode.h"
#include "OpenFile.h"
#include "DirEntry.h"

#define USER_OPEN_FILE_TABLE_SIZE 256

using namespace std;

class User {
public:

    /**
     * 构造函数
     * @param uid user id
     * @param gid group id
     */
    User(int uid, int gid);

    /**
     * 打开文件，也可打开目录文件
     * @param path 文件路径，相对路径或绝对路径均可
     * @param flags 读写标记
     * @return 打开的file descriptor，-1表示出错
     */
    int Open(const char *path, int flags);

    /**
     * 关闭文件
     * @param fd 要关闭的文件描述符
     * @return 0表示成功，-1表示错误
     */
    int Close(int fd);

    /**
     * 创建文件，也可创建目录文件
     * @param path 目标路径
     * @param mode 各用户的mode, 包括文件类型（目录/文件）、RWX权限等
     * @return 创建的file descriptor，-1表示出错
     */
    int Create(const char *path, int mode);

    /**
     * @brief 设置硬链接
     * @param srcPath 被链接的文件路径，可以是目录文件
     * @param dstPath 目标文件路径
     * @return 0表示成功，-1表示错误
     */
    int Link(const char *srcPath, const char *dstPath);

    /**
     * @brief 删除文件，如果链接数大于1，不会真的删除，而是取消path的链接
     * @param path 目标路径
     * @return 0表示成功，-1表示错误
     */
    int Unlink(const char *path);

    /**
     * @brief 读取相应的文件数据
     * @param fd file descriptor
     * @param buffer 读取缓冲区
     * @param size 读取字节数
     * @return 返回实际读取字节数，-1表示错误
     */
    int Read(int fd, char* buffer, int size);

    /**
     * @brief 将数据写入文件
     * @param fd file descriptor
     * @param buffer 写入缓冲区
     * @param size 写入字节数
     * @return 返回实际读取字节数，-1表示错误
     */
    int Write(int fd, char* buffer, int size);


    /**
     * @brief 设置读写指针位置
     * @param fd file descriptor
     * @param offset 偏移量，可以为负
     * @param fromWhere 基准位置
     * @return 0表示成功，-1表示失败
     */
    int Seek(int fd, int offset, int fromWhere);

    /**
     * @brief 获取文件信息
     * @param path 路径
     * @param stat_buf 返回值缓冲区
     * @return 0表示成功，-1表示失败
     */
    int GetStat(const char *path, struct FileStat* stat_buf);

    /**
     * @brief 根据文件inode获取信息
     * @param inodeIdx inode序号
     * @param stat_buf 返回值缓冲区
     * @return 0表示成功，-1表示失败
     */
    static int GetInodeStat(int inodeIdx, struct FileStat* stat_buf);

    MemInode* currentWorkDir;
    int uid;
    int gid;
    OpenFile userOpenFileTable[USER_OPEN_FILE_TABLE_SIZE]; ///< 用户打开的文件列表，如果表项的f_inode == nullptr表示未被占用

    static User* userPtr; ///< 全局user指针，在init函数中被初始化，在shutdown函数中释放
private:
    /**
     * @brief 获取空闲的表项
     * @return 返回空闲的index，没找到为-1
     */
    int GetEmptyEntry();

    /**
     * 以read权限打开path最低端的目录文件，并解析目标文件名
     * @param path 路径
     * @param currentDirFile path最低端的目录文件
     * @param nameBuffer 目标文件名
     * @param nameBufferIdx 目标文件名长度
     * @return 0表示成功，-1表示错误
     */
    int GetDirFile(const char *path, OpenFile& currentDirFile, char* nameBuffer, int& nameBufferIdx);
};

#endif //MOFS_USER_H
