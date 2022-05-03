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
#include "include/device/DeviceManager.h"

using namespace std;

int main(int argc, char* argv[]) {
    // 解析输入参数
    string imagePath;

    // 初始化
    DeviceManager::deviceManager.OpenImage(imagePath);

    /// @todo SuperBlock的加载应当在确认选择的映象文件已经建立了文件系统之后执行，或者通过mkfs建立。
    DeviceManager::deviceManager.LoadSuperBlock(&SuperBlock::superBlock);

    return 0;
}