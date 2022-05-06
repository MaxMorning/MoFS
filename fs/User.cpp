/**
 * @file User.cpp
 * @brief 
 * @author 韩孟霖
 * @date 2022/5/6
 * @license GPL v3
 */

#include "../include/User.h"
#include "../include/DirEntry.h"
#include "../utils/Diagnose.h"
#include "../include/OpenFile.h"

#define BLOCK_SIZE 512


bool NameComp(const char* name1, const char* name2, int size) {
    for (int i = 0; i < size; ++i) {
        if (name1[i] != name2[i]) {
            return false;
        }
    }
    return true;
}
/**
 * 根据名称找到相应文件的DiskInode
 * @param nameBuffer 名称
 * @param bufferSize 名称长度
 * @param dirFile 目录文件
 * @return DiskInode序号，-1为错误
 */
int SearchFileInodeByName(char* nameBuffer, int bufferSize, OpenFile& dirFile) {
    // 逐块读取并查找
    DirEntry entries[BLOCK_SIZE / sizeof(DirEntry)];

    int readByteCnt = 0;
    while (true) {
        readByteCnt = dirFile.Read((char* )entries, BLOCK_SIZE);
        if (readByteCnt <= 0) {
            break;
        }

        for (int i = 0; i < readByteCnt / sizeof(DirEntry); ++i) {
            if (entries[i].m_ino >= 0 && NameComp(nameBuffer, entries[i].m_name, bufferSize) == 0) {
                return entries[i].m_ino;
            }
        }
    }

    return -1;
}

/**
 * @brief 删除dirFile中名为nameBuffer的项
 * @param nameBuffer 文件名
 * @param bufferSize name长度
 * @param dirFile 目录文件
 * @return 0为成功，-1为错误
 */
int RemoveEntryInDirFile(char* nameBuffer, int bufferSize, OpenFile& dirFile) {
    // 逐块读取并查找
    DirEntry entries[BLOCK_SIZE / sizeof(DirEntry)];

    int readByteCnt = 0;
    while (true) {
        readByteCnt = dirFile.Read((char* )entries, BLOCK_SIZE);
        if (readByteCnt <= 0) {
            break;
        }

        for (int i = 0; i < readByteCnt / sizeof(DirEntry); ++i) {
            if (entries[i].m_ino >= 0 && NameComp(nameBuffer, entries[i].m_name, bufferSize) == 0) {
                entries[i].m_ino = -1; // 将ino标记为-1，设置为空闲

                // 写回
                dirFile.Seek(-1 * readByteCnt, SEEK_CUR);
                dirFile.Write((char*)entries, readByteCnt);
                return 0;
            }
        }
    }

    return -1;
}

/**
 * @brief 将名为nameBuffer的项插入dirFile中
 * @param nameBuffer 文件名
 * @param bufferSize name长度
 * @param dirFile 目录文件
 * @return 0为成功，-1为错误
 */
int InsertEntryInDirFile(char* nameBuffer, int bufferSize, int inodeIdx, OpenFile& dirFile) {
    // 逐块读取并查找
    DirEntry entries[BLOCK_SIZE / sizeof(DirEntry)];

    int readByteCnt = 0;
    while (true) {
        readByteCnt = dirFile.Read((char* )entries, BLOCK_SIZE);
        if (readByteCnt <= 0) {
            break;
        }

        for (int i = 0; i < readByteCnt / sizeof(DirEntry); ++i) {
            if (entries[i].m_ino < 0) {
                entries[i].m_ino = inodeIdx;
                memcpy(entries[i].m_name, nameBuffer, bufferSize);

                // 写回
                if (-1 == dirFile.Seek(-1 * readByteCnt, SEEK_CUR)) {
                    return -1;
                }
                if (-1 == dirFile.Write((char*)entries, readByteCnt)) {
                    return -1;
                }
                return 0;
            }
        }
    }

    // 没找到空闲的，在尾端插入
    DirEntry newFileEntry{};
    newFileEntry.m_ino = inodeIdx;
    memcpy(newFileEntry.m_name, nameBuffer, NAME_MAX_LENGTH);

    if (-1 == dirFile.Seek(1, SEEK_END)) {
        return -1;
    }
    if (-1 == dirFile.Write((char*)&newFileEntry, sizeof(DirEntry))) {
        return -1;
    }

    return 0;
}

int User::GetDirFile(const string& path, OpenFile& currentDirFile, char* nameBuffer, int& nameBufferIdx) {
    int currentDiskInodeIndex;
    int pathStrIdx = 0;

    if (path[0] == '/') {
        // 绝对路径
        currentDiskInodeIndex = SuperBlock::superBlock.s_rootInode;
        ++pathStrIdx;
    }
    else {
        // 相对路径
        currentDiskInodeIndex = this->currentWorkDir->i_number;
    }

    if (-1 == OpenFile::OpenFileFactory(currentDirFile, currentDiskInodeIndex, this->uid, this->gid, FileFlags::FREAD)) {
        return -1;
    }


    nameBufferIdx = 0;
    for (; pathStrIdx < path.length(); ++pathStrIdx) {
        if (path[pathStrIdx] == '/') {
            assert(pathStrIdx > 0);

            if (path[pathStrIdx - 1] == '/') {
                // 连续的//，视作一个
                continue;
            }
            else {
                if (currentDirFile.IsDirFile()) {
                    // 在这个文件里找到nameBuffer的inode值
                    currentDiskInodeIndex = SearchFileInodeByName(nameBuffer, nameBufferIdx, currentDirFile);
                    if (currentDiskInodeIndex == -1) {
                        Diagnose::PrintError("Dir not found.");
                        return -1;
                    }
                    else {
                        currentDirFile.Close();
                        if (-1 == OpenFile::OpenFileFactory(currentDirFile, currentDiskInodeIndex, this->uid, this->gid, FileFlags::FREAD)) {
                            return -1;
                        }
                    }
                }
                else {
                    Diagnose::PrintError(string(nameBuffer, nameBufferIdx) + " is not a dir file.");
                    currentDirFile.Close();
                    return -1;
                }
            }
        }
        else {
            // 是一个字符，加入buffer中
            if (nameBufferIdx >= NAME_MAX_LENGTH) {
                // 超出名字最大长度
                Diagnose::PrintError("Path invalid.");
                return -1;
            }

            nameBuffer[nameBufferIdx] = path[pathStrIdx];
            ++nameBufferIdx;
        }
    }

    return 0;
}

int User::Open(const string& path, int flags) {
    int emptyIndex = this->GetEmptyEntry();
    if (emptyIndex == -1) {
        Diagnose::PrintError("No more empty file descriptor left.");
        return -1;
    }

    OpenFile currentDirFile;
    char nameBuffer[NAME_MAX_LENGTH];
    int nameBufferIdx;

    if (this->GetDirFile(path, currentDirFile, nameBuffer, nameBufferIdx) == -1) {
        return -1;
    }

    // 函数正常结束后，nameBuffer中保存有最后一个文件的名称
    if (currentDirFile.IsDirFile()) {
        // 在这个文件里找到nameBuffer的inode值
        int currentDiskInodeIndex = SearchFileInodeByName(nameBuffer, nameBufferIdx, currentDirFile);
        if (currentDiskInodeIndex == -1) {
            Diagnose::PrintError("Dir not found.");
            return -1;
        }
        else {
            currentDirFile.Close();
            if (-1 == OpenFile::OpenFileFactory(currentDirFile, currentDiskInodeIndex, this->uid, this->gid, flags)) {
                return -1;
            }
        }
    }
    else {
        Diagnose::PrintError(string(nameBuffer, nameBufferIdx) + " is not a dir file.");
        currentDirFile.Close();
        return -1;
    }

    // 现在 currentDirFile 即为要求打开的文件结构
    this->userOpenFileTable[emptyIndex] = currentDirFile;
    return emptyIndex;
}

int User::Create(const string &path, int mode) {
    int emptyIndex = this->GetEmptyEntry();
    if (emptyIndex == -1) {
        Diagnose::PrintError("No more empty file descriptor left.");
        return -1;
    }

    OpenFile currentDirFile;
    char nameBuffer[NAME_MAX_LENGTH];
    int nameBufferIdx;

    if (this->GetDirFile(path, currentDirFile, nameBuffer, nameBufferIdx) == -1) {
        return -1;
    }

    // 函数正常结束后，nameBuffer中保存有最后一个文件的名称
    if (!currentDirFile.IsDirFile()) {
        Diagnose::PrintError(string(nameBuffer, nameBufferIdx) + " is not a dir file.");
        currentDirFile.Close();
        return -1;
    }

    // 此时currentDirFile是以read权限打开的，需要检查write权限
    if (!currentDirFile.CheckFlags(FileFlags::FWRITE, this->uid, this->gid)) {
        Diagnose::PrintError("Don't have permission to write.");
        return -1;
    }
    currentDirFile.f_inode->i_flag = FileFlags::FWRITE;

    int newDiskInode = SuperBlock::superBlock.AllocDiskInode();
    if (newDiskInode == -1) {
        Diagnose::PrintError("No more inode available");
        return -1;
    }

    if (-1 == InsertEntryInDirFile(nameBuffer, nameBufferIdx, newDiskInode, currentDirFile)) {
        return -1;
    }

    // 创建完成后打开
    if (-1 == OpenFile::OpenFileFactory(this->userOpenFileTable[emptyIndex], newDiskInode, this->uid, this->gid, FileFlags::FWRITE)) {
        return -1;
    }

    // 设置权限
    this->userOpenFileTable[emptyIndex].f_inode->i_mode = mode;

    return emptyIndex;
}


int User::GetEmptyEntry() {
    for (int i = 0; i < USER_OPEN_FILE_TABLE_SIZE; ++i) {
        if (userOpenFileTable[i].f_inode == nullptr) {
            return i;
        }
    }
    return -1;
}

int User::Close(int fd) {
    if (this->userOpenFileTable[fd].Close() == -1) {
        return -1;
    }

    this->userOpenFileTable[fd].f_inode = nullptr;
    return 0;
}

int User::Read(int fd, char *buffer, int size) {
    if (fd >= USER_OPEN_FILE_TABLE_SIZE || this->userOpenFileTable[fd].f_inode == nullptr) {
        Diagnose::PrintError("Bad file descriptor.");
        return -1;
    }

    return this->userOpenFileTable[fd].Read(buffer, size);
}

int User::Write(int fd, char *buffer, int size) {
    if (fd >= USER_OPEN_FILE_TABLE_SIZE || this->userOpenFileTable[fd].f_inode == nullptr) {
        Diagnose::PrintError("Bad file descriptor.");
        return -1;
    }

    return this->userOpenFileTable[fd].Write(buffer, size);
}

int User::Seek(int fd, int offset, int fromWhere) {
    if (fd >= USER_OPEN_FILE_TABLE_SIZE || this->userOpenFileTable[fd].f_inode == nullptr) {
        Diagnose::PrintError("Bad file descriptor.");
        return -1;
    }

    return this->userOpenFileTable[fd].Seek(offset, fromWhere);
}

int User::Unlink(const string &path) {
    OpenFile currentDirFile;
    char nameBuffer[NAME_MAX_LENGTH];
    int nameBufferIdx;

    if (this->GetDirFile(path, currentDirFile, nameBuffer, nameBufferIdx) == -1) {
        return -1;
    }

    // 函数正常结束后，nameBuffer中保存有最后一个文件的名称
    if (!currentDirFile.IsDirFile()) {
        Diagnose::PrintError(string(nameBuffer, nameBufferIdx) + " is not a dir file.");
        currentDirFile.Close();
        return -1;
    }

    // 此时currentDirFile是以read权限打开的，需要检查write权限
    if (!currentDirFile.CheckFlags(FileFlags::FWRITE, this->uid, this->gid)) {
        Diagnose::PrintError("Don't have permission to write.");
        return -1;
    }
    currentDirFile.f_inode->i_flag = FileFlags::FWRITE;

    int diskInode = SearchFileInodeByName(nameBuffer, nameBufferIdx, currentDirFile);
    if (diskInode == -1) {
        return -1;
    }

    // 在父目录处删除这一条记录
    if (-1 == RemoveEntryInDirFile(nameBuffer, nameBufferIdx, currentDirFile)) {
        Diagnose::PrintError("Cannot delete entry from parent dir.");
        return -1;
    }

    // 处理对应的DiskInode
    DiskInode unlinkedDiskInode;
    if (-1 == DiskInode::DiskInodeFactory(diskInode, unlinkedDiskInode)) {
        return -1;
    }

    unlinkedDiskInode.d_nlink--;

    // link数为0，需要释放DiskInode
    if (unlinkedDiskInode.d_nlink == 0) {
        if (-1 == unlinkedDiskInode.ReleaseBlocks()) {
            return -1;
        }

        if (-1 == SuperBlock::superBlock.ReleaseInode(diskInode)) {
            return -1;
        }
    }
    return 0;
}

User::User(int uid, int gid) {
    this->uid = uid;
    this->gid = gid;
    memset(this->userOpenFileTable, 0, USER_OPEN_FILE_TABLE_SIZE * sizeof(OpenFile));

    MemInode::MemInodeFactory(SuperBlock::superBlock.s_rootInode, this->currentWorkDir);
}

