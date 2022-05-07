/**
 * @file DirEntry.h
 * @brief 
 * @author 韩孟霖
 * @date 2022/5/3
 * @license GPL v3
 */

#ifndef MOFS_DIRENTRY_H
#define MOFS_DIRENTRY_H

/// 目录项名称的最长字节数
#define NAME_MAX_LENGTH 28

/**
 * @brief 目录项，每个目录文件包含若干个目录项
 */
class DirEntry {
public:
    int m_ino;                        ///< 目录项对应的DiskInode
    char m_name[NAME_MAX_LENGTH];     ///< 文件名
};

#endif //MOFS_DIRENTRY_H
