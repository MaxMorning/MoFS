/**
 * @file main.cpp
 * @brief main函数文件
 * @mainpage main
 * @author 韩孟霖
 * @date 2022/05/03
 * @license GPL v3
 */
#include <string>
#include <ctime>

#include "include/SuperBlock.h"
#include "include/MemInode.h"
#include "include/device/DeviceManager.h"
#include "include/User.h"
#include "utils/Diagnose.h"
#include "include/Primitive.h"
#include "include/MoFSErrno.h"
#include "include/CLI.h"
#include "utils/CmdTools.h"


#ifndef _WIN32
#include "FTP/common.h"
#endif

using namespace std;


int uid, gid;

int InitSystem() {
    memset(MemInode::systemMemInodeTable, 0, sizeof(MemInode) * SYSTEM_MEM_INODE_NUM);
    memset(User::userTable, 0, sizeof(int*) * MAX_USER_NUM);
    User::userPtr = new User{uid, gid};
    User::userTable[0] = User::userPtr;

    MoFSErrno = 0;
    return 0;
}

int shutdown() {
    // 释放userPtr
    for (User* & userPtr : User::userTable) {
        if (userPtr != nullptr) {
            delete userPtr;
        }
    }


    // 写回SuperBlock
    return DeviceManager::deviceManager.StoreSuperBlock(&SuperBlock::superBlock);
}

int main(int argc, char* argv[]) {
    // 解析输入参数
    string imagePath;
    int parse_image_path_result = get_str_argument(argc, argv, "--img", imagePath);
    if (parse_image_path_result == PARSE_ERR_KEY_NOT_FOUND) {
        Diagnose::PrintError("Cannot find arg : img.");
        exit(-1);
    }
    if (parse_image_path_result == PARSE_ERR_INVALID_VALUE) {
        Diagnose::PrintError("Cannot parse arg : img.");
        exit(-1);
    }

    // Make fs 相关
    bool shouldMakeFS = (PARSE_SUCCESS == get_argument(argc, argv, "--mkfs", nullptr, nullptr));


    // 初始化
    DeviceManager::deviceManager.OpenImage(imagePath.c_str());

    if (shouldMakeFS) {
        int disk_size = 10;
        int parse_size_result = get_argument(argc, argv, "--size", "%d", &disk_size);
        if (parse_size_result == PARSE_ERR_INVALID_VALUE) {
            Diagnose::PrintError("Cannot parse arg : size.");
            exit(-1);
        }

        int inode_num = 2048;
        int parse_inode_result = get_argument(argc, argv, "--inode", "%d", &inode_num);
        if (parse_inode_result == PARSE_ERR_INVALID_VALUE) {
            Diagnose::PrintError("Cannot parse arg : inode.");
            exit(-1);
        }

        if (-1 == SuperBlock::MakeFS(disk_size * 1024 * 1024, inode_num)) {
            Diagnose::PrintError("Initial : Make FS failed.");
            exit(-1);
        }
        Diagnose::PrintLog("Initial : Make FS success.");
    }
    else {
        if (-1 == DeviceManager::deviceManager.LoadSuperBlock(&SuperBlock::superBlock)) {
            Diagnose::PrintError("Initial : Load SuperBlock failed.");
            exit(-1);
        }
        Diagnose::PrintLog("Initial : Load SuperBlock success.");
    }

    InitSystem();

    bool ftp_mode = (PARSE_SUCCESS == get_argument(argc, argv, "--ftp", nullptr, nullptr));
    bool cli_mode = (PARSE_SUCCESS == get_argument(argc, argv, "--cli", nullptr, nullptr));
    if (cli_mode) {
        // 控制台模式
        int loop_time = -1;
        if (PARSE_ERR_INVALID_VALUE == get_argument(argc, argv, "--loop", "%d", &loop_time)) {
            Diagnose::PrintError("Cannot parse arg : loop.");
            exit(-1);
        }

        infinite_loop(cin, loop_time);

        if (-1 == shutdown()) {
            Diagnose::PrintError("Shutdown error.");
            exit(-1);
        }
    }
    else if (ftp_mode) {
        // FTP模式
#ifdef _WIN32
        Diagnose::PrintError("FTP not supported under Windows platform.");
        exit(-1);
#else
        int port = 8021;
        if (PARSE_ERR_INVALID_VALUE == get_argument(argc, argv, "--port", "%d", &port)) {
            Diagnose::PrintError("Cannot parse arg : port.");
            exit(-1);
        }
        server(8021);

        // 写回和释放在进程收到SIGTERM时释放
        Diagnose::PrintLog("Server done.");
#endif
    }
    else {
        Diagnose::PrintError("Unrecognized mode.");
        exit(-1);
    }

    return 0;
}