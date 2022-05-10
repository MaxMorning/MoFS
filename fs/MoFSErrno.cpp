/**
 * @file MoFSErrno.cpp
 * @brief 类似于Linux的errno
 * @author 韩孟霖
 * @date 2022/5/9
 * @license GPL v3
 */

#include "../include/MoFSErrno.h"

int MoFSErrno;

char ErrnoMsg[MAX_ERRNO][MAX_MSG_LENGTH] = {
        "Success",
        "Operation not permitted",
        "No such file or directory",
        "Bad file descriptor",
        "Cannot allocate memory",
        "File exists",
        "Not a directory",
        "Is a directory",
        "Too many open files in system",
        "Too many open files for user",
        "File too large",
        "No space left on device",
        "Illegal seek",
        "File name too long",
        "Directory not empty",
        "No more inode available",
        "Block device error",
        "File is not closed",
        "",
        "Unknown error"
};
