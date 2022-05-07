/**
 * @file main.cpp
 * @brief main函数文件
 * @mainpage main
 * @author 韩孟霖
 * @date 2022/05/03
 * @license GPL v3
 */
#include <string>

#include "include/SuperBlock.h"
#include "include/MemInode.h"
#include "include/device/DeviceManager.h"
#include "include/User.h"
#include "utils/Diagnose.h"

using namespace std;


int InitSystem() {
    memset(MemInode::systemMemInodeTable, 0, sizeof(MemInode) * SYSTEM_MEM_INODE_NUM);
    return 0;
}

int main(int argc, char* argv[]) {
    // 解析输入参数
    string imagePath = "1.img";

    // 初始化
    DeviceManager::deviceManager.OpenImage(imagePath);
    InitSystem();

    SuperBlock::MakeFS(32 * 1024 * 1024, 2048);
    /// @todo SuperBlock的加载应当在确认选择的映象文件已经建立了文件系统之后执行，或者通过mkfs建立。
//    DeviceManager::deviceManager.LoadSuperBlock(&SuperBlock::superBlock);
    User user{0, 0};
    int fd = user.Create("/hello.txt", 0777);
    char content[] = "HML testing";
    int writeByteCnt = user.Write(fd, content, sizeof(content));
    Diagnose::PrintLog("Store " + to_string(writeByteCnt) + " byte(s).");

    char readBuffer[1024];
    if (-1 == user.Seek(fd, 0, SEEK_SET)) {
        return -1;
    }

    int readByteCnt = user.Read(fd, readBuffer, 1024);
    Diagnose::PrintLog("Load " + to_string(readByteCnt) + " byte(s), content " + string(readBuffer, readByteCnt));

    user.Close(fd);

    return 0;
}