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
#include "include/Primitive.h"
#include "include/MoFSErrno.h"

using namespace std;


int uid, gid;

int InitSystem() {
    memset(MemInode::systemMemInodeTable, 0, sizeof(MemInode) * SYSTEM_MEM_INODE_NUM);
    User::userPtr = new User{uid, gid};
    MoFSErrno = 0;
    return 0;
}

int shutdown() {
    // 释放userPtr
    delete User::userPtr;

    // 写回SuperBlock
    return DeviceManager::deviceManager.StoreSuperBlock(&SuperBlock::superBlock);
}

int main(int argc, char* argv[]) {
    // 解析输入参数
    char imagePath[] = "1.img";

    // 初始化
    DeviceManager::deviceManager.OpenImage(imagePath);

//#define MAKEFS

#ifdef MAKEFS
    SuperBlock::MakeFS(32 * 1024 * 1024, 2048);
    /// @todo SuperBlock的加载应当在确认选择的映象文件已经建立了文件系统之后执行，或者通过mkfs建立。
//    DeviceManager::deviceManager.LoadSuperBlock(&SuperBlock::superBlock);
    User user{0, 0};
    int fd = user.Create("/hello.txt", MemInode::IALLOC | MemInode::IFMT | 0777);
    char content[] = "HML testing";
    int writeByteCnt = user.Write(fd, content, sizeof(content));
    Diagnose::PrintLog("Store " + to_string(writeByteCnt) + " byte(s).");

    char readBuffer[1024];
    if (-1 == user.Seek(fd, 2, SEEK_SET)) {
        return -1;
    }
    int readByteCnt = user.Read(fd, readBuffer, 1024);
    Diagnose::PrintLog("Load " + to_string(readByteCnt) + " byte(s), content " + string(readBuffer, readByteCnt));

    user.Close(fd);
#elif defined(READ_TEST)
//    SuperBlock::MakeFS(32 * 1024 * 1024, 2048);
    DeviceManager::deviceManager.LoadSuperBlock(&SuperBlock::superBlock);
    User user{0, 0};
    int fd = user.Open("/hello.txt", FileFlags::FREAD | FileFlags::FWRITE);

    if (-1 == user.Seek(fd, 0, SEEK_END)) {
        return -1;
    }

    char content[] = "HML testing";
    int writeByteCnt = user.Write(fd, content, sizeof(content));
    Diagnose::PrintLog("Store " + to_string(writeByteCnt) + " byte(s).");

    char readBuffer[1024];
    if (-1 == user.Seek(fd, 2, SEEK_SET)) {
        return -1;
    }
    int readByteCnt = user.Read(fd, readBuffer, 1024);
    Diagnose::PrintLog("Load " + to_string(readByteCnt) + " byte(s), content " + string(readBuffer, readByteCnt));

    user.Close(fd);

#else
//    SuperBlock::MakeFS(16 * 1024 * 1024, 2048);
    DeviceManager::deviceManager.LoadSuperBlock(&SuperBlock::superBlock);

    InitSystem();

    if (-1 == mofs_unlink("/hello2")) {
        Diagnose::PrintErrno("Cannot unlink /hello2");
    }
    else {
        Diagnose::PrintLog("/hello2 unlinked.");
    }

    int mk_result = mofs_mkdir("/hello2", 0777);

    if (mk_result != -1) {
        Diagnose::PrintLog("hello2 dir created");
    }
    else {
        Diagnose::PrintErrno("Cannot create /hello2");
    }


    int fd2 = mofs_creat("/hello2/2.txt", 0777);

    if (fd2 != -1) {
        Diagnose::PrintLog("/hello2/2.txt created");
    }

    mofs_close(fd2);

    if (-1 == mofs_link("/hello2/2.txt", "3.txt")) {
        Diagnose::PrintErrno("Cannot link 3.txt");
    }


    int fd3 = mofs_open("3.txt", O_WRONLY, 0);

    int write_byte_cnt = mofs_write(fd3, (void *) "Hello Morning!", 20);
    Diagnose::PrintLog("Write " + to_string(write_byte_cnt) + " byte(s) to 3.txt.");
    mofs_close(fd3);

    if (-1 == mofs_unlink("3.txt")) {
        Diagnose::PrintError("Cannot unlink 3.txt");
    }
    else {
        Diagnose::PrintLog("3.txt unlinked.");
    }

    fd2 = mofs_open("/hello2/2.txt", O_RDONLY, 0);
    char read_buffer[64];
    int read_byte_cnt = mofs_read(fd2, read_buffer, 64);
    Diagnose::PrintLog("Read " + to_string(read_byte_cnt) + " byte(s) from 2.txt, content : " + string{read_buffer});

    mofs_close(fd2);

    if (-1 == mofs_unlink("/hello2/2.txt")) {
        Diagnose::PrintErrno("Cannot unlink /hello2/2.txt");
    }
    else {
        Diagnose::PrintLog("/hello2/2.txt unlinked.");
    }


#endif

    if (-1 == shutdown()) {
        Diagnose::PrintError("Shutdown error.");
        exit(-1);
    }
    return 0;
}