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


/**
 * @brief 文件信息结构体，对标UNIX中 sys/stat.h 的 stat 结构体，有删改
 */
struct FileStat {
    int st_ino;     ///< 文件的inode序号
    unsigned int st_mode;    ///< 文件的mode（RWX权限等）
    int st_nlink;   ///< 硬链接数
    int st_uid;     ///< user id
    int st_gid;     ///< group id
    int st_size;    ///< 文件字节数
    int st_atime;   ///< 最近访问时间
    int st_mtime;   ///< 最近修改时间

};
#endif //MOFS_DIRENTRY_H
