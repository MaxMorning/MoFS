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
    char imagePath[] = "1.img";

    // 初始化
    DeviceManager::deviceManager.OpenImage(imagePath);

    DeviceManager::deviceManager.LoadSuperBlock(&SuperBlock::superBlock);

    InitSystem();

    infinite_loop(cin, -1);

    if (-1 == shutdown()) {
        Diagnose::PrintError("Shutdown error.");
        exit(-1);
    }
    return 0;
}