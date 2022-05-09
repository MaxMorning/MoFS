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
#include "../include/MoFSErrno.h"

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
    if (open_fd < 0) {
        if (MoFSErrno == 2) {
            // 文件不存在，或者路径有误
            if ((oflags & O_CREAT) == O_CREAT) {
                // 调用者要求创建
                MoFSErrno = 0;
                open_fd = User::userPtr->Create(pathname, mode & 0777);
                if (open_fd < 0) {
                    // 这时候还找不到文件或目录，说明路径有问题
                    return -1;
                }

                // 成功创建
                return open_fd;
            }
            else {
                return -1;
            }
        }
        else {
            // 遇到了其它错误
            return -1;
        }
    }

    // 这里开始，open_fd >= 0，需要处理append和directory
    if ((oflags & O_DIRECTORY) == O_DIRECTORY) {
        if (!User::userPtr->userOpenFileTable[open_fd].IsDirFile()) {
            MoFSErrno = 6;
            return User::userPtr->Close(open_fd);
        }
    }

    if ((oflags & O_APPEND) == O_APPEND) {
        mofs_lseek(open_fd, 1, SEEK_END);
    }

    return open_fd;
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

int mofs_stat(const char *pathname, struct FileStat *statbuf) {
    return User::userPtr->GetStat(pathname, statbuf);
}

int mofs_inode_stat(int inodeIndex, struct FileStat *statbuf) {
    return User::GetInodeStat(inodeIndex, statbuf);
}
