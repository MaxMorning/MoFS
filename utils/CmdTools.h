/**
 * @file CmdTools.h
 * @brief 解析命令行工具
 * @author 韩孟霖
 * @date 2022/5/14
 * @license GPL v3
 */

#ifndef MOFS_CMDTOOLS_H
#define MOFS_CMDTOOLS_H
#include <string>

#define PARSE_SUCCESS 0
#define PARSE_ERR_KEY_NOT_FOUND (-1)
#define PARSE_ERR_INVALID_VALUE (-2)

using namespace std;

/**
    @param      argc        : the count of origin input args
    @param      argv        : origin input args from user type in
    @param      arg_key     : the key of arguments asked for, include "--"
    @param      arg_form    : the format of value(eg. %d means integer, %f means float, "%s" means string(need to alloc enough memory firstly))
    @param      arg_value   : use this pointer to return parsed value

    @return
        0       Parse value successfully
        -1      Key not found
        -2      Input value invalid
*/
int get_argument(   int argc, char* argv[],
                    char* arg_key, char* arg_form,
                    void* arg_value);

int get_str_argument(   int argc, char* argv[],
                        char* arg_key, string& value);

#endif //MOFS_CMDTOOLS_H
