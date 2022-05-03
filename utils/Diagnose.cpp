/**
 * @file Diagnose.cpp
 * @brief 
 * @author 韩孟霖
 * @date 2022/5/4
 * @license GPL v3
 */

#include <iostream>

#include "Diagnose.h"

using namespace std;

void Diagnose::PrintError(const std::string& errorInfo) {
    cerr << "[ERROR]\t" << errorInfo << endl;
}

void Diagnose::PrintLog(const std::string& logInfo) {
    cout << "[LOG]\t" << logInfo << endl;
}
