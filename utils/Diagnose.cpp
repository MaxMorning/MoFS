/**
 * @file Diagnose.cpp
 * @brief 
 * @author 韩孟霖
 * @date 2022/5/4
 * @license GPL v3
 */

#include <iostream>

#include "Diagnose.h"
#include "../include/MoFSErrno.h"

using namespace std;

void Diagnose::PrintError(const std::string& errorInfo) {
#ifdef PRINT_ERROR_IN_LOW_LAYERS
    cerr << "[ERROR]\t" << errorInfo << endl;
#endif
}

void Diagnose::PrintLog(const std::string& logInfo) {
    cout << "[LOG]\t" << logInfo << endl;
}

void Diagnose::PrintErrno(const string &errorInfo) {
    cout << "[ERROR]\t" << errorInfo << ", errno(" << MoFSErrno << "):" << ErrnoMsg[MoFSErrno] << endl;
}
