/**
 * @file Primitive.cpp
 * @brief 原语实现文件
 * @mainpage Primitive
 * @author 韩孟霖
 * @date 2022/05/09
 * @license GPL v9
 */

#include "../include/Primitive.h"
#include "../include/User.h"

int mofs_creat(const char *pathname, int mode) {
    // 掩码处理传入的mode参数，只保留最低9bit
    mode &= 0777;

    // 创建文件
    return User::userPtr->Create(pathname, MemInode::IALLOC | MemInode::IFMT | mode);
}

int mofs_mkdir(const char *pathname, int mode) {
    // 掩码处理传入的mode参数，只保留最低9bit
    mode &= 0777;

    // 创建目录文件
    int fd = User::userPtr->Create(pathname, MemInode::IALLOC | MemInode::IFDIR | mode);
    if (fd < 0) {
        return -1;
    }

    return User::userPtr->Close(fd);
}

int mofs_open(const char *pathname, int oflags,int mode) {
    int open_fd = User::userPtr->Open(pathname, oflags & 0x3);
    throw "Not implemented!";
}

int mofs_read(int fd, void *buffer, int count) {
    if (count < 0) {
        return -1;
    }

    return User::userPtr->Read(fd, (char *) buffer, count);
}

int mofs_write(int fd, void *buffer, int count) {
    if (count < 0) {
        return -1;
    }

    return User::userPtr->Write(fd, (char *) buffer, count);
}

int mofs_lseek(int fd, int offset, int whence) {
    return User::userPtr->Seek(fd, offset, whence);
}

int mofs_close(int fd) {
    return User::userPtr->Close(fd);
}

int mofs_link(const char *srcpath, const char *dstpath) {
    return User::userPtr->Link(srcpath, dstpath);
}

int mofs_unlink(const char *pathname) {
    return User::userPtr->Unlink(pathname);
}
