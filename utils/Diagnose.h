/**
 * @file Diagnose.h
 * @brief 负责输出日志和报错
 * @author 韩孟霖
 * @date 2022/5/4
 * @license GPL v3
 */

#ifndef MOFS_DIAGNOSE_H
#define MOFS_DIAGNOSE_H

#include <string>

/**
 * @brief 诊断类，所有输出都通过该类实现
 */
class Diagnose {
public:
    /**
     * @brief 向stderr打印报错信息
     * @param errorInfo 报错信息
     */
    static void PrintError(const std::string& errorInfo);

    /**
     * @brief 向stdout打印日志信息
     * @param logInfo 日志信息
     */
    static void PrintLog(const std::string& logInfo);
};


#endif //MOFS_DIAGNOSE_H
