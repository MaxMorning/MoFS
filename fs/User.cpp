/**
 * @file User.cpp
 * @brief 
 * @author 韩孟霖
 * @date 2022/5/6
 * @license GPL v3
 */
#include <cassert>
#include <ctime>

#include "../include/User.h"
#include "../include/DirEntry.h"
#include "../utils/Diagnose.h"
#include "../include/MoFSErrno.h"

#define BLOCK_SIZE 512

// 默认情况下
User* User::userPtr = nullptr;
User* User::userTable[MAX_USER_NUM];

bool NameComp(const char* name1, const char* name2, int size) {
    for (int i = 0; i < size; ++i) {
        if (name1[i] != name2[i]) {
            return false;
        }
    }
    if (name1[size] != '\0' || name2[size] != '\0') {
        return false;
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
    if (bufferSize == 0) {
        return dirFile.f_inode->i_number;
    }
    // 逐块读取并查找
    DirEntry entries[BLOCK_SIZE / sizeof(DirEntry)];

    int readByteCnt = 0;
    dirFile.Seek(0, SEEK_SET);
    while (true) {
        readByteCnt = dirFile.Read((char* )entries, BLOCK_SIZE);
        if (readByteCnt <= 0) {
            break;
        }

        for (int i = 0; i < readByteCnt / sizeof(DirEntry); ++i) {
            if (entries[i].m_ino > 0 && NameComp(nameBuffer, entries[i].m_name, bufferSize)) {
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
    dirFile.Seek(0, SEEK_SET);
    while (true) {
        readByteCnt = dirFile.Read((char* )entries, BLOCK_SIZE);
        if (readByteCnt <= 0) {
            break;
        }

        for (int i = 0; i < readByteCnt / sizeof(DirEntry); ++i) {
            if (entries[i].m_ino > 0 && NameComp(nameBuffer, entries[i].m_name, bufferSize)) {
                entries[i].m_ino = -1; // 将ino标记为-1，设置为空闲

                // 写回
                dirFile.Seek(-1 * readByteCnt, SEEK_CUR);
                dirFile.Write((char*)entries, readByteCnt);

                // 将更新后的inode写回磁盘
                if (-1 == dirFile.f_inode->StoreToDisk(dirFile.f_inode->i_lastAccessTime, time(nullptr))) {
                    return -1;
                }

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
    dirFile.Seek(0, SEEK_SET);
    while (true) {
        readByteCnt = dirFile.Read((char* )entries, BLOCK_SIZE);
        if (readByteCnt <= 0) {
            break;
        }

        for (int i = 0; i < readByteCnt / sizeof(DirEntry); ++i) {
            if (entries[i].m_ino <= 0) {
                entries[i].m_ino = inodeIdx;
                memset(entries[i].m_name, 0, NAME_MAX_LENGTH);
                memcpy(entries[i].m_name, nameBuffer, bufferSize);

                // 写回
                if (-1 == dirFile.Seek(-1 * readByteCnt, SEEK_CUR)) {
                    return -1;
                }
                if (-1 == dirFile.Write((char*)entries, readByteCnt)) {
                    return -1;
                }

                // 将更新后的inode写回磁盘
                if (-1 == dirFile.f_inode->StoreToDisk(dirFile.f_inode->i_lastAccessTime, time(nullptr))) {
                    return -1;
                }

                return 0;
            }
        }
    }

    // 没找到空闲的，在尾端插入
    DirEntry newFileEntry{};
    newFileEntry.m_ino = inodeIdx;
    memset(newFileEntry.m_name, 0, NAME_MAX_LENGTH);
    memcpy(newFileEntry.m_name, nameBuffer, NAME_MAX_LENGTH);

    if (-1 == dirFile.Seek(0, SEEK_END)) {
        return -1;
    }
    if (-1 == dirFile.Write((char*)&newFileEntry, sizeof(DirEntry))) {
        return -1;
    }

    // 将更新后的inode写回磁盘
    if (-1 == dirFile.f_inode->StoreToDisk(dirFile.f_inode->i_lastAccessTime, time(nullptr))) {
        return -1;
    }

    return 0;
}

int User::GetDirFile(const char *path, OpenFile& currentDirFile, char* nameBuffer, int& nameBufferIdx) {
    int currentDiskInodeIndex;
    int pathStrIdx = 0;

    if (path[0] == '/') {
        // 绝对路径
        currentDiskInodeIndex = SuperBlock::superBlock.s_rootInode;
        ++pathStrIdx;
    }
    else if (path[0] == '.' && path[1] == '/') {
        pathStrIdx = 2;
        currentDiskInodeIndex = this->userOpenFileTable[this->currentWorkDir].f_inode->i_number;
    }
    else {
        // 相对路径
        currentDiskInodeIndex = this->userOpenFileTable[this->currentWorkDir].f_inode->i_number;
    }

    if (-1 == OpenFile::OpenFileFactory(currentDirFile, currentDiskInodeIndex, this->uid, this->gid, FileFlags::FREAD)) {
        return -1;
    }


    memset(nameBuffer, 0, NAME_MAX_LENGTH);
    nameBufferIdx = 0;
    for (; path[pathStrIdx] != '\0'; ++pathStrIdx) {
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
                        MoFSErrno = 2;
                        Diagnose::PrintError("Dir not found.");
                        return -1;
                    }
                    else {
                        currentDirFile.Close(false);
                        if (-1 == OpenFile::OpenFileFactory(currentDirFile, currentDiskInodeIndex, this->uid, this->gid, FileFlags::FREAD)) {
                            return -1;
                        }

                        memset(nameBuffer, 0, NAME_MAX_LENGTH);
                        nameBufferIdx = 0;
                    }
                }
                else {
                    MoFSErrno = 6;
                    Diagnose::PrintError("this is not a dir file.");
                    currentDirFile.Close(false);
                    return -1;
                }
            }
        }
        else {
            // 是一个字符，加入buffer中
            if (nameBufferIdx >= NAME_MAX_LENGTH) {
                // 超出名字最大长度
                MoFSErrno = 13;
                Diagnose::PrintError("Path invalid.");
                return -1;
            }

            nameBuffer[nameBufferIdx] = path[pathStrIdx];
            ++nameBufferIdx;
        }
    }

    return 0;
}

int User::Open(const char *path, int flags) {
    int emptyIndex = this->GetEmptyEntry();
    if (emptyIndex == -1) {
        MoFSErrno = 9;
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
            MoFSErrno = 2;
            Diagnose::PrintError("No such file.");
            currentDirFile.Close(false);
            return -1;
        }
        else {
            currentDirFile.Close(false);
            if (-1 == OpenFile::OpenFileFactory(currentDirFile, currentDiskInodeIndex, this->uid, this->gid, flags)) {
                return -1;
            }
        }
    }
    else {
        MoFSErrno = 6;
        Diagnose::PrintError("Parent of opening file is not a dir file.");
        currentDirFile.Close(false);
        return -1;
    }

    // 现在 currentDirFile 即为要求打开的文件结构
    this->userOpenFileTable[emptyIndex] = currentDirFile;
    return emptyIndex;
}

int User::Create(const char *path, int mode) {
    int emptyIndex = this->GetEmptyEntry();
    if (emptyIndex == -1) {
        MoFSErrno = 9;
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
        MoFSErrno = 6;
        Diagnose::PrintError("Parent of creating file is not a dir file.");
        currentDirFile.Close(false);
        return -1;
    }

    // 此时currentDirFile是以read权限打开的，需要检查write权限
    if (!currentDirFile.CheckFlags(FileFlags::FWRITE, this->uid, this->gid)) {
        MoFSErrno = 1;
        Diagnose::PrintError("Don't have permission to write.");
        return -1;
    }
    currentDirFile.f_flag |= FileFlags::FWRITE;

    // 检查currentDirFile中是否已经存在同名文件
    int checkExist = SearchFileInodeByName(nameBuffer, nameBufferIdx, currentDirFile);
    if (checkExist != -1) {
        // 当前Dir已经存在，创建失败
        MoFSErrno = 5;
        Diagnose::PrintError("File already exist");
        currentDirFile.Close(false);
        return -1;
    }

    int newDiskInode = SuperBlock::superBlock.AllocDiskInode();
    if (newDiskInode == -1) {
        // errno 已经在AllocDiskInode设置
        Diagnose::PrintError("No more inode available");
        return -1;
    }

    if (-1 == InsertEntryInDirFile(nameBuffer, nameBufferIdx, newDiskInode, currentDirFile)) {
        return -1;
    }

    // 创建完成后打开
    // 不能从OpenFileFactory构造，因为此时newDiskInode尚未写入磁盘，而OpenFileFactory会从磁盘中加载DiskInode

    // 新建一个MemInode
    MemInode* memInodePtr;
    if (-1 == MemInode::MemInodeNotInit(memInodePtr)) {
        return -1;
    }

    this->userOpenFileTable[emptyIndex].f_inode = memInodePtr;
    this->userOpenFileTable[emptyIndex].f_flag = FileFlags::FWRITE;
    this->userOpenFileTable[emptyIndex].f_offset = 0;

    // MemInode初始化
//    memInodePtr->i_flag = INodeFlag::IUPD | INodeFlag::IACC;
    // 设置权限
    memInodePtr->i_mode = mode;
    memInodePtr->i_count = 1;
    memInodePtr->i_nlink = 1;
    memInodePtr->i_dev = 0;
    memInodePtr->i_number = newDiskInode;
    memInodePtr->i_uid = this->uid;
    memInodePtr->i_gid = this->gid;
    memInodePtr->i_size = 0;
    memset(memInodePtr->i_addr, -1, 10 * sizeof(int));

    // 将inode 写回磁盘
    int currentTime = time(nullptr);
    if (-1 == memInodePtr->StoreToDisk(currentTime, currentTime)) {
        return -1;
    }
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
    if (fd < 0 || fd >= USER_OPEN_FILE_TABLE_SIZE || this->userOpenFileTable[fd].f_inode == nullptr) {
        Diagnose::PrintError("Bad file descriptor.");
        MoFSErrno = 3;
        return -1;
    }

    if (fd == this->currentWorkDir) {
        // 不能close工作目录的fd
        MoFSErrno = 17;
        return -1;
    }

    if (this->userOpenFileTable[fd].Close(true) == -1) {
        return -1;
    }

    this->userOpenFileTable[fd].f_inode = nullptr;
    return 0;
}

int User::Read(int fd, char *buffer, int size) {
    if (fd < 0 || fd >= USER_OPEN_FILE_TABLE_SIZE || this->userOpenFileTable[fd].f_inode == nullptr) {
        MoFSErrno = 3;
        Diagnose::PrintError("Bad file descriptor.");
        return -1;
    }

    return this->userOpenFileTable[fd].Read(buffer, size);
}

int User::Write(int fd, char *buffer, int size) {
    if (fd < 0 || fd >= USER_OPEN_FILE_TABLE_SIZE || this->userOpenFileTable[fd].f_inode == nullptr) {
        MoFSErrno = 3;
        Diagnose::PrintError("Bad file descriptor.");
        return -1;
    }

    int temp = this->userOpenFileTable[fd].Write(buffer, size);

    return temp;
}

int User::Seek(int fd, int offset, int fromWhere) {
    if (fd < 0 || fd >= USER_OPEN_FILE_TABLE_SIZE || this->userOpenFileTable[fd].f_inode == nullptr) {
        MoFSErrno = 3;
        Diagnose::PrintError("Bad file descriptor.");
        return -1;
    }

    return this->userOpenFileTable[fd].Seek(offset, fromWhere);
}


int User::Link(const char *srcPath, const char *dstPath) {
    // 找到srcPath的inode
    OpenFile currentDirFile;
    char nameBuffer[NAME_MAX_LENGTH];
    int nameBufferIdx;

    if (this->GetDirFile(srcPath, currentDirFile, nameBuffer, nameBufferIdx) == -1) {
        return -1;
    }

    // 函数正常结束后，nameBuffer中保存有最后一个文件的名称
    if (!currentDirFile.IsDirFile()) {
        MoFSErrno = 6;
        Diagnose::PrintError("This is not a dir file.");
        currentDirFile.Close(false);
        return -1;
    }

    int srcDiskInode = SearchFileInodeByName(nameBuffer, nameBufferIdx, currentDirFile);
    if (srcDiskInode == -1) {
        MoFSErrno = 2;
        Diagnose::PrintError("File not exist.");
        return -1;
    }



    // 处理目标路径
    if (this->GetDirFile(dstPath, currentDirFile, nameBuffer, nameBufferIdx) == -1) {
        return -1;
    }

    // 函数正常结束后，nameBuffer中保存有最后一个文件的名称
    if (!currentDirFile.IsDirFile()) {
        MoFSErrno = 6;
        Diagnose::PrintError("Parent of linking file is not a dir file.");
        currentDirFile.Close(false);
        return -1;
    }

    // 此时currentDirFile是以read权限打开的，需要检查write权限
    if (!currentDirFile.CheckFlags(FileFlags::FWRITE, this->uid, this->gid)) {
        // errno 在 CheckFlags中设置
        Diagnose::PrintError("Don't have permission to write.");
        return -1;
    }
    currentDirFile.f_flag |= FileFlags::FWRITE;

    // 检查currentDirFile中是否已经存在同名文件
    int checkExist = SearchFileInodeByName(nameBuffer, nameBufferIdx, currentDirFile);
    if (checkExist != -1) {
        // 当前Dir已经存在，创建失败
        MoFSErrno = 5;
        Diagnose::PrintError("File already exist");
        return -1;
    }

    if (-1 == InsertEntryInDirFile(nameBuffer, nameBufferIdx, srcDiskInode, currentDirFile)) {
        // 出错了，但不应该。更低层次的API应当已经设置好了。
        Diagnose::PrintError("Cannot insert entry into dir");
        return -1;
    }

    // 更新inode
    OpenFile linkOpenFile;
    if (-1 == OpenFile::OpenFileFactory(linkOpenFile, srcDiskInode, this->uid, this->gid, 0)) {
        return -1;
    }
    linkOpenFile.f_inode->i_nlink++;

    return linkOpenFile.Close(true);
}


int User::Unlink(const char *path) {
    OpenFile currentDirFile;
    char nameBuffer[NAME_MAX_LENGTH];
    int nameBufferIdx;

    if (this->GetDirFile(path, currentDirFile, nameBuffer, nameBufferIdx) == -1) {
        return -1;
    }

    // 函数正常结束后，nameBuffer中保存有最后一个文件的名称
    if (!currentDirFile.IsDirFile()) {
        MoFSErrno = 6;
        Diagnose::PrintError("Parent of unlinking file is not a dir file.");
        currentDirFile.Close(false);
        return -1;
    }

    // 此时currentDirFile是以read权限打开的，需要检查write权限
    if (!currentDirFile.CheckFlags(FileFlags::FWRITE, this->uid, this->gid)) {
        // errno 已在 CheckFlags 设置
        Diagnose::PrintError("Don't have permission to write.");
        return -1;
    }
    currentDirFile.f_flag |= FileFlags::FWRITE;

    int diskInode = SearchFileInodeByName(nameBuffer, nameBufferIdx, currentDirFile);
    if (diskInode == -1) {
        MoFSErrno = 2;
        Diagnose::PrintError("File not exist.");
        return -1;
    }

    // 处理对应的DiskInode，应该操作OpenFile而非直接操作DiskInode
    OpenFile unlinkedOpenFile;
    if (-1 == OpenFile::OpenFileFactory(unlinkedOpenFile, diskInode, this->uid, this->gid, FileFlags::FREAD | FileFlags::FWRITE)) {
        return -1;
    }

    if (unlinkedOpenFile.f_inode->i_count > 1) {
        // 当前还有其它OpenFile打开这个文件，不让删，需要报错
        MoFSErrno = 17;
        unlinkedOpenFile.Close(false);
        return -1;
    }

    unlinkedOpenFile.f_inode->i_nlink--;
    // link数为0，需要释放DiskInode
    if (unlinkedOpenFile.f_inode->i_nlink == 0) {
        // 检查该DiskInode是否是目录，如果是，检查是否为空。不允许删除有内容的目录
        if (unlinkedOpenFile.HaveFilesInDir()) {
            MoFSErrno = 14;
            Diagnose::PrintError("Cannot delete a dir with files.");
            return -1;
        }

        if (-1 == unlinkedOpenFile.f_inode->ReleaseBlocks()) {
            return -1;
        }

        if (-1 == SuperBlock::superBlock.ReleaseInode(diskInode)) {
            return -1;
        }
    }
    else {
        // 该文件仍然有连接，需要将更新后的 inode 写回磁盘中
        int currentTime = time(nullptr);
        unlinkedOpenFile.f_inode->StoreToDisk(currentTime, currentTime);
    }

    // 在父目录处删除这一条记录
    if (-1 == RemoveEntryInDirFile(nameBuffer, nameBufferIdx, currentDirFile)) {
        // errno 在 RemoveEntryInDirFile 已设置
        Diagnose::PrintError("Cannot delete entry from parent dir.");
        return -1;
    }

    return unlinkedOpenFile.Close(true);
}

User::User(int uid, int gid) {
    this->uid = uid;
    this->gid = gid;
    memset(this->userOpenFileTable, 0, USER_OPEN_FILE_TABLE_SIZE * sizeof(OpenFile));

    this->currentWorkDir = this->Open("/", FileFlags::FREAD | FileFlags::FWRITE);
}

int User::GetStat(const char *path, struct FileStat *stat_buf) {
    // 基于path找到对应inode
    OpenFile currentDirFile;
    char nameBuffer[NAME_MAX_LENGTH];
    int nameBufferIdx;

    if (this->GetDirFile(path, currentDirFile, nameBuffer, nameBufferIdx) == -1) {
        return -1;
    }

    // 函数正常结束后，nameBuffer中保存有最后一个文件的名称
    if (!currentDirFile.IsDirFile()) {
        MoFSErrno = 6;
        Diagnose::PrintError("This is not a dir file.");
        currentDirFile.Close(false);
        return -1;
    }

    int targetInode = SearchFileInodeByName(nameBuffer, nameBufferIdx, currentDirFile);
    if (targetInode == -1) {
        MoFSErrno = 2;
        Diagnose::PrintError("File not exist.");
        return -1;
    }

    return User::GetInodeStat(targetInode, stat_buf);
}

int User::GetInodeStat(int inodeIdx, struct FileStat *stat_buf) {
    MemInode* memInodePtr;
    if (-1 == MemInode::MemInodeFactory(inodeIdx, memInodePtr)) {
        return -1;
    }

    stat_buf->st_ino = inodeIdx;
    stat_buf->st_mode = memInodePtr->i_mode;
    stat_buf->st_nlink = memInodePtr->i_nlink;
    stat_buf->st_uid = memInodePtr->i_uid;
    stat_buf->st_gid = memInodePtr->i_gid;
    stat_buf->st_size = memInodePtr->i_size;
    stat_buf->st_atime = memInodePtr->i_lastAccessTime;
    stat_buf->st_mtime = memInodePtr->i_lastModifyTime;

    memInodePtr->Close(false);
    return 0;
}

int User::ChangeDir(const char *new_dir) {
    int workDirFD = this->Open(new_dir, FileFlags::FWRITE | FileFlags::FREAD);
    if (-1 == workDirFD) {
        return -1;
    }

    // 这里不使用User::Close来关闭原先的工作目录，是因为不希望将访问时间写回工作目录
    if (-1 == this->userOpenFileTable[this->currentWorkDir].Close(false)) {
        return -1;
    }
    this->userOpenFileTable[this->currentWorkDir].f_inode = nullptr; // 释放fd

    this->currentWorkDir = workDirFD;
    return 0;
}

User::~User() {
    // 关闭所有打开的文件
    for (int i = 0; i < USER_OPEN_FILE_TABLE_SIZE; ++i) {
        if (i != this->currentWorkDir && this->userOpenFileTable[i].f_inode != nullptr) {
            // 这里不管关闭是否成功了，失败了也没法处理
            this->Close(i);
        }
    }

    // 关闭工作目录
    if (currentWorkDir >= 0) {
        this->userOpenFileTable[this->currentWorkDir].Close(false);
    }
}

User * User::ChangeUser(int uid, int gid) {
    // 在userTable中找
    for (User* ptr : User::userTable) {
        if (ptr != nullptr) {
            if (ptr->uid == uid && ptr->gid == gid) {
                return ptr;
            }
        }
    }

    // 没找到
    for (User* & ptr : User::userTable) {
        if (ptr == nullptr) {
            ptr = new User{uid, gid};
            return ptr;
        }
    }

    MoFSErrno = 18;
    return nullptr;
}

