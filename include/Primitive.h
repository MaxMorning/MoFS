/**
 * @file Primitive.h
 * @brief 原语头文件
 * @mainpage Primitive
 * @author 韩孟霖
 * @date 2022/05/09
 * @license GPL v3
 * @note 原语层函数应当兼容SUSv3标准
 */

#ifndef MOFS_PRIMITIVE_H
#define MOFS_PRIMITIVE_H

#include "OpenFile.h"
#include "DirEntry.h"

/**
 * @brief 创建普通文件，并以只读方式打开
 * @param pathname 创建的路径
 * @param mode 本用户、用户组、其他用户的RWX权限
 * @return 文件打开的fd，-1为失败
 */
int mofs_creat(const char *pathname, int mode);

/**
 * @brief 创建目录
 * @param pathname 创建的路径
 * @param mode 本用户、用户组、其他用户的RWX权限
 * @return 0为成功，-1为失败
 */
int mofs_mkdir(const char *pathname, int mode);

/**
 * @brief 打开文件，当然也可以打开目录文件
 * @param pathname 路径
 * @param oflags 选项
 * @param mode 当oflags中有O_CREAT时，创建的文件的RWX权限
 * @return 文件打开的fd，-1为失败
 */
int mofs_open(const char *pathname, int oflags,int mode);

/**
 * @brief 读取指定字节数
 * @param fd 文件描述符
 * @param buffer 存放读取数据的缓冲区
 * @param count 要求读的字节数
 * @return 实际读取的字节数，-1表示出错
 */
int mofs_read(int fd, void *buffer, int count);

/**
 * @brief 写入指定的字节数
 * @param fd 文件描述符
 * @param buffer 存放待读取数据的缓冲区
 * @param count 要求写的字节数
 * @return 实际读取的字节数，-1表示出错
 */
int mofs_write(int fd, void *buffer, int count);

/**
 * @brief 移动文件的读写指针
 * @param fd 文件描述符
 * @param offset 移动的字节数，允许为负数
 * @param whence 从哪里开始移动
 * @return 新的读写指针位置，-1表示出错
 */
int mofs_lseek(int fd, int offset, int whence);

/**
 * @brief 关闭文件
 * @param fd 要关闭的文件描述符
 * @return 0为成功，-1为失败
 */
int mofs_close(int fd);

/**
 * @brief 建立硬连接
 * @param oldpath 硬连接来源路径
 * @param newpath 硬连接目标地址
 * @return 0为成功，-1为失败
 */
int mofs_link(const char *srcpath, const char *dstpath);

/**
 * @brief 取消硬连接，当连接数小于1时会删除文件
 * @param pathname 路径
 * @return 0为成功，-1为失败
 */
int mofs_unlink(const char *pathname);

/**
 * @brief 获取文件信息
 * @param pathname 路径
 * @param statbuf 存放返回信息的缓冲区
 * @return 0为成功，-1为失败
 */
int mofs_stat(const char *pathname, struct FileStat* statbuf);

/**
 * @brief 根据文件inode获取信息
 * @param inodeIndex inode序号
 * @param statbuf 返回值缓冲区
 * @return 0表示成功，-1表示失败
 */
int mofs_inode_stat(int inodeIndex, struct FileStat *statbuf);

// 以下为mofs_open函数中oflags可使用的选项
// 以下三项必须三选一
const int MOFS_RDONLY = FileFlags::MOFS_READ;  ///< 0001 只读
const int MOFS_WRONLY = FileFlags::MOFS_WRITE; ///< 0010 只写
const int MOFS_RDWR = MOFS_RDONLY | MOFS_WRONLY; ///< 0011 读写

// 以下各项可选
const int MOFS_CREAT = 0x4;                ///< 0100, 如果待打开的文件不存在，则创建
const int MOFS_APPEND = 0x8;               ///< 1000, 将读写指针设置在结尾
const int MOFS_DIRECTORY = 0x10;           ///< 0001 0000, 如果打开的文件不是目录文件，则返回-1

// 以下为mofs_open函数中mode可使用的选项，仅在oflags有O_CREAT时有效
const int MOFS_IRUSR = 0400;           ///< 100 000 000 本用户可读
const int MOFS_IWUSR = 0200;           ///< 010 000 000 本用户可写
const int MOFS_IXUSR = 0100;           ///< 001 000 000 本用户可执行

const int MOFS_IRGRP = MOFS_IRUSR >> 3;   ///< 000 100 000 同组用户可读
const int MOFS_IWGRP = MOFS_IWUSR >> 3;   ///< 000 010 000 同组用户可写
const int MOFS_IXGRP = MOFS_IXUSR >> 3;   ///< 000 001 000 同组用户可执行

const int MOFS_IROTH = MOFS_IRGRP >> 3;   ///< 其他用户可读
const int MOFS_IWOTH = MOFS_IWGRP >> 3;   ///< 其他用户可写
const int MOFS_IXOTH = MOFS_IXGRP >> 3;   ///< 其他用户可执行
#endif //MOFS_PRIMITIVE_H
