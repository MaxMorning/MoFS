/**
 * @file CmdTools.cpp
 * @brief 解析命令行工具
 * @author 韩孟霖
 * @date 2022/5/14
 * @license GPL v3
 * @note 代码风格不一致的原因是这两个函数系复用本人计网课程作业
 */

#include <cstring>


#include "CmdTools.h"

int get_argument(   int argc, char* argv[],
                    char* arg_key, char* arg_form,
                    void* arg_value)
{
    int arg_idx = -1;
    argc = arg_value == nullptr ? argc : argc - 1;
    for (int i = 1; i < argc; ++i) {
        if (strcmp(arg_key, argv[i]) == 0) {
            // arg key match
            arg_idx = i;
            break;
        }
    }

    if (arg_idx == -1) {
        return PARSE_ERR_KEY_NOT_FOUND;
    }

    if (arg_value == nullptr) {
        return PARSE_SUCCESS;
    }
    else if (argv[arg_idx + 1][0] == '-') {
        return PARSE_ERR_INVALID_VALUE;
    }

    int sscanf_success = sscanf(argv[arg_idx + 1], arg_form, arg_value);

    if (sscanf_success == EOF) {
        return PARSE_ERR_INVALID_VALUE;
    }

    return PARSE_SUCCESS;
}

int get_str_argument(   int argc, char* argv[],
                        char* arg_key, string& value) {
    int arg_idx = -1;
    --argc;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(arg_key, argv[i]) == 0) {
            // arg key match
            arg_idx = i;
            break;
        }
    }

    if (arg_idx == -1) {
        return PARSE_ERR_KEY_NOT_FOUND;
    }

    if (argv[arg_idx + 1][0] == '-') {
        return PARSE_ERR_INVALID_VALUE;
    }

    value = string(argv[arg_idx + 1]);

    return PARSE_SUCCESS;
}