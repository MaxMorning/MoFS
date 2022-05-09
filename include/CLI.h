/**
 * @file CLI.h
 * @brief 命令行界面头文件, 理论上从原语层之上，就是用户代码而非内核代码了，可以使用STL / new之类
 * @author 韩孟霖
 * @date 2022/5/9
 * @license GPL v3
 */
#ifndef MOFS_CLI_H
#define MOFS_CLI_H

#include <iostream>

using namespace std;

/**
 * @brief cli无限循环，当用户输入exit指令或者遇到严重错误时返回
 * @param input_stream 输入流，可以是cin，也可以是ifstream
 */
void infinite_loop(istream &input_stream, int loop_time);

#endif //MOFS_CLI_H
