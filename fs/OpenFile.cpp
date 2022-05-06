/**
 * @file OpenFile.cpp
 * @brief 
 * @author 韩孟霖
 * @date 2022/5/6
 * @license GPL v3
 */
#include <ctime>

#include "../utils/Diagnose.h"
#include "../include/OpenFile.h"

#define BLOCK_SIZE 512

int OpenFile::Read(char *buffer, int size) {
    int returnValue = this->f_inode->Read(this->f_offset, buffer, size);

    if (returnValue >= 0) {
        this->f_offset += returnValue;
        this->f_lastModifyTime = time(nullptr);
    }

    return returnValue;
}

int OpenFile::Write(char *buffer, int size) {
    int returnValue = this->f_inode->Write(this->f_offset, buffer, size);

    if (returnValue > 0) {
        this->f_offset += returnValue;
    }

    return returnValue;
}

int OpenFile::Close() {
    --(this->f_inode->i_count);

    if (this->f_inode->i_count <= 0) {
        // 将inode写回磁盘中
        return this->f_inode->Close(this->f_lastAccessTime, this->f_lastModifyTime);
    }

    return 0;
}

int OpenFile::Open(int flag, MemInode *inode, int uid, int gid) {
    // 检查权限
    this->f_inode = inode;
    if (!this->CheckFlags(flag, uid, gid)) {
        Diagnose::PrintError("File permission not granted.");
        return -1;
    }

    // 权限检查通过
    this->f_flag = flag;
    inode->i_count++;

    this->f_count = 1;
    this->f_offset = 0;

    this->f_lastAccessTime = time(nullptr);

    return 0;
}

bool OpenFile::CheckFlags(int flag, int uid, int gid) {
    unsigned int reverseFlag = 0;
    if (flag & FileFlags::FREAD) {
        reverseFlag = 0x4;
    }

    if (flag & FileFlags::FWRITE) {
        reverseFlag = reverseFlag | 0x2;
    }

    bool permissionChecked = false;
    if (this->f_inode->i_uid == uid) {
        // 同一个用户
        unsigned int maskedMode = this->f_inode->i_mode & MemInode::IRWXU;
        maskedMode = (maskedMode >> 6) & reverseFlag;

        if (maskedMode == reverseFlag) {
            permissionChecked = true;
        }
    }
    else if (this->f_inode->i_gid == gid) {
        // 同组用户
        unsigned int maskedMode = this->f_inode->i_mode & MemInode::IRWXG;
        maskedMode = (maskedMode >> 3) & reverseFlag;

        if (maskedMode == reverseFlag) {
            permissionChecked = true;
        }
    }
    else {
        // 其它用户
        unsigned int maskedMode = this->f_inode->i_mode & MemInode::IRWXO;
        maskedMode = maskedMode & reverseFlag;

        if (maskedMode == reverseFlag) {
            permissionChecked = true;
        }
    }

    return permissionChecked;
}

int OpenFile::Seek(int offset, int fromWhere) {
    switch (fromWhere) {
        case SEEK_SET:
        {
            if (offset < 0) {
                return -1;
            }

            if (offset > (6 + 128 * 2 + 128 * 128 * 2) * BLOCK_SIZE) {
                return -1;
            }

            this->f_offset = offset;
        }
            break;

        case SEEK_CUR:
        {
            int newOffset = this->f_offset + offset;

            if (newOffset < 0) {
                return -1;
            }

            if (newOffset > (6 + 128 * 2 + 128 * 128 * 2) * BLOCK_SIZE) {
                return -1;
            }

            this->f_offset = newOffset;
        }
            break;

        case SEEK_END:
        {
            int newOffset = this->f_inode->i_size + offset;

            if (newOffset < 0) {
                return -1;
            }

            if (newOffset > (6 + 128 * 2 + 128 * 128 * 2) * BLOCK_SIZE) {
                return -1;
            }

            this->f_offset = newOffset;
        }
            break;

        default:
            return -1;
    }
    return 0;
}

int OpenFile::OpenFileFactory(OpenFile &openFile, int diskInodeIdx, int uid, int gid, int flags) {
    MemInode::MemInodeFactory(diskInodeIdx, openFile.f_inode);

    return openFile.Open(flags, openFile.f_inode, uid, gid);
}

bool OpenFile::IsDirFile() {
    return (this->f_inode->i_mode & MemInode::IFMT) == MemInode::IFDIR;
}