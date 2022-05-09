/**
 * @file MoFSErrno.h
 * @brief 类似于Linux的errno，当出错时被设置
 * @author 韩孟霖
 * @date 2022/5/9
 * @license GPL v3
 */

#ifndef MOFS_MOFSERRNO_H
#define MOFS_MOFSERRNO_H

#define MAX_ERRNO 20
#define MAX_MSG_LENGTH 32

/// 对标errno，但具体值和Linux不一致
extern int MoFSErrno;

/// 每个MoFSErrno对应的错误描述
extern char ErrnoMsg[MAX_ERRNO][MAX_MSG_LENGTH];

#endif //MOFS_MOFSERRNO_H
