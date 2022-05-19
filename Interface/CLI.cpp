/**
 * @file CLI.cpp
 * @brief 命令行界面实现文件
 * @author 韩孟霖
 * @date 2022/5/9
 * @license GPL v3
 */

#include <iostream>
#include <string>
#include <unordered_map>
#include <iomanip>
#include <sstream>

#include "../include/CLI.h"
#include "../include/User.h"
#include "../utils/Diagnose.h"
#include "../include/Primitive.h"

using namespace std;

// []中参数为必须，{}中参数为可选，第二个:后为默认值
#define MKFS_MAP_VALUE          0       ///< 格式化:                                     mkfs [大小(MB): int] [最大inode数: int]
#define FFORMAT_MAP_VALUE       0       ///< 格式化:                                     fformat [大小(MB): int] [最大inode数: int]
#define LS_MAP_VALUE            1       ///< 展示目录下文件:                              ls [路径: str: 当前目录]
#define MKDIR_MAP_VALUE         2       ///< 建立目录:                                   mkdir [路径目录名: str] [权限: oct_int]
#define FCREAT_MAP_VALUE        3       ///< 创建文件(返回fd: int):                       fcreat [路径名: str] [权限: oct_int]
#define FOPEN_MAP_VALUE         4       ///< 打开文件(返回fd: int):                       fopen [路径名: str] [要求: str] {权限: oct_int}     (要求：rw, r+, etc.)
#define FCLOSE_MAP_VALUE        5       ///< 关闭文件:                                   fclose [fd: int]
#define FREAD_MAP_VALUE         6       ///< 读取文件内容(返回读取字节数: int, 内容: str):   fread [fd: int] [读取字节数: int]
#define FWRITE_MAP_VALUE        7       ///< 写入文件(返回实际读取的字节数: int):            fwrite [fd: int] [写入字节数: int] [写入内容(不允许有空格和换行): str]
#define FLSEEK_MAP_VALUE        8       ///< 设置读写指针(返回设置的指针位置: int):          flseek [fd: int] [偏移量: int] [起始位置: str]   (起始位置: SET / CUR / END)
#define FDELETE_MAP_VALUE       9       ///< 删除(取消链接):                              fdelete [路径名: str]
#define MVIN_MAP_VALUE          10      ///< 拷贝入(返回拷贝字节数: int):                  mvin [外部路径名: str] [内部路径名: str]
#define MVOUT_MAP_VALUE         11      ///< 拷贝出(返回拷贝字节数: int):                  mvout [内部路径名: str] [外部路径名: str]
#define CHGUSR_MAP_VALUE        12      ///< 切换用户:                                   chgusr [uid: int] [gid: int]
#define CD_MAP_VALUE            13      ///< 切换工作目录:                                cd [路径名: str]
#define LINK_MAP_VALUE          14      ///< 创建硬链接:                                 link [源路径名: str] [目标路径名: str]
#define EXIT_MAP_VALUE          15      ///< 退出程序                                    exit
#define HELP_MAP_VALUE          16      ///< 帮助与提示:                                 help

/**
 * @brief 处理一条指令
 * @param input_stream 输入流
 * @param command_enum_mapping 映射
 * @return 返回-1则需要退出循环
 */
int process_command(const string &command, stringstream &input_stream,
                    const unordered_map<string, int> &command_enum_mapping);

string currentWorkDir;
string cli_header;


void infinite_loop(istream &input_stream, int loop_time) {
    // 初始化工作
    cli_header = "[User(" + to_string(User::userPtr->uid) + ", " + to_string(User::userPtr->gid) + ") ";
    currentWorkDir = "/";

    const unordered_map<string, int> command_enum_mapping{
            {"mkfs", MKFS_MAP_VALUE},
            {"fformat", FFORMAT_MAP_VALUE},
            {"ls", LS_MAP_VALUE},
            {"mkdir", MKDIR_MAP_VALUE},
            {"fcreat", FCREAT_MAP_VALUE},
            {"fopen", FOPEN_MAP_VALUE},
            {"fclose", FCLOSE_MAP_VALUE},
            {"fread", FREAD_MAP_VALUE},
            {"fwrite", FWRITE_MAP_VALUE},
            {"flseek", FLSEEK_MAP_VALUE},
            {"fdelete", FDELETE_MAP_VALUE},
            {"mvin", MVIN_MAP_VALUE},
            {"mvout", MVOUT_MAP_VALUE},
            {"chgusr", CHGUSR_MAP_VALUE},
            {"cd", CD_MAP_VALUE},
            {"link", LINK_MAP_VALUE},
            {"unlink", FDELETE_MAP_VALUE},
            {"exit", EXIT_MAP_VALUE},
            {"help", HELP_MAP_VALUE}
    };

    string command;

    if (loop_time < 0) {
        while (true) {
            cout << cli_header << currentWorkDir << "]$ ";
            string line;
            getline(cin, line);
            if (line.length() == 0) {
                continue;
            }
            stringstream string_stream{line};
            string_stream >> command;
            if (-1 == process_command(command, string_stream, command_enum_mapping)) {
                break;
            }
        }
    }
    else {
        for (int i = 0; i < loop_time; ++i) {
            cout << cli_header;
            string line;
            getline(cin, line);
            if (line.length() == 0) {
                continue;
            }
            stringstream string_stream{line};
            string_stream >> command;
            if (-1 == process_command(command, string_stream, command_enum_mapping)) {
                break;
            }
        }
    }
}

void print_list(const char* pathname);

#define TRANS_BUFFER_SIZE 2048

int process_command(const string &command, stringstream &input_stream,
                    const unordered_map<string, int> &command_enum_mapping) {
    auto iter = command_enum_mapping.find(command);
    if (iter == command_enum_mapping.end()) {
        cout << "Unrecognized command." << endl;
        return 0;
    }

    switch (iter->second) {
        case MKFS_MAP_VALUE: {
            int total_bytes = -1, max_inode_num = -1;
            input_stream >> total_bytes >> max_inode_num;
            if (total_bytes == -1 || max_inode_num == -1) {
                Diagnose::PrintError("Need more args.");
                return 0;
            }

            if (-1 == SuperBlock::MakeFS(total_bytes * 1024 * 1024, max_inode_num)) {
                Diagnose::PrintErrno("Cannot make file system");
                return -1;
            }

            // 关闭所有打开的文件
            for (auto & i : MemInode::systemMemInodeTable) {
                i.i_used = 0;
            }

            for (auto & i : User::userPtr->userOpenFileTable) {
                i.f_inode = nullptr;
            }

            delete User::userPtr;

            // 默认用户为0, 0
            User::userPtr = new User{0, 0};

            cout << "Make file system success.\n" << endl;
        }
        break;


        case LS_MAP_VALUE: {
            string pathname = "./";
            input_stream >> pathname;
            print_list(pathname.c_str());
        }
        break;

        case MKDIR_MAP_VALUE: {
            int mode = 0777;
            string pathname;
            input_stream >> pathname;
            input_stream >> oct >> mode >> dec;

            mode &= 0777;
            if (mofs_mkdir(pathname.c_str(), mode) == -1) {
                Diagnose::PrintErrno("Cannot make dir");
                return 0;
            }
        }
        break;

        case FCREAT_MAP_VALUE: {
            int mode = 0777;
            string pathname;
            input_stream >> pathname;
            input_stream >> oct >> mode >> dec;

            int fd = mofs_creat(pathname.c_str(), mode);
            if (fd < 0) {
                Diagnose::PrintErrno("Cannot create file");
                return 0;
            }

            cout << "File created, fd = " << fd << endl;
        }
        break;

        case FOPEN_MAP_VALUE: {
            string pathname, flags;
            input_stream >> pathname >> flags;
            int mode = 0777;

            int fd = -1;
            if (flags == "r") {
                fd = mofs_open(pathname.c_str(), MOFS_RDONLY, 0);
            }
            else if (flags == "w") {
                input_stream >> oct >> mode >> dec;

                fd = mofs_open(pathname.c_str(), MOFS_WRONLY | MOFS_CREAT, mode);
            }
            else if (flags == "a") {
                input_stream >> oct >> mode >> dec;

                fd = mofs_open(pathname.c_str(), MOFS_WRONLY | MOFS_APPEND | MOFS_CREAT, mode);
            }
            else if (flags == "r+") {
                fd = mofs_open(pathname.c_str(), MOFS_RDWR, 0);
            }
            else if (flags == "w+") {
                input_stream >> oct >> mode >> dec;

                fd = mofs_open(pathname.c_str(), MOFS_RDWR | MOFS_CREAT, mode);
            }
            else if (flags == "a+") {
                input_stream >> oct >> mode >> dec;

                fd = mofs_open(pathname.c_str(), MOFS_RDWR | MOFS_APPEND | MOFS_CREAT, mode);
            }
            else {
                Diagnose::PrintErrno("Cannot recognize flags : " + flags);
                return 0;
            }

            if (fd < 0) {
                Diagnose::PrintErrno("Cannot open file with " + flags);
                return 0;
            }

            cout << "File opened, fd = " << fd << endl;
        }
        break;

        case FCLOSE_MAP_VALUE: {
            int fd = -1;
            input_stream >> fd;
            if (fd == -1) {
                Diagnose::PrintError("Need more args.");
                return 0;
            }

            if (mofs_close(fd) == -1) {
                Diagnose::PrintErrno("Cannot close file");
                return 0;
            }
        }
        break;

        case FREAD_MAP_VALUE: {
            int fd = -1, count = -1;
            input_stream >> fd >> count;
            if (fd == -1 || count == -1) {
                Diagnose::PrintError("Need more args.");
                return 0;
            }

            char* read_buffer = new char[count];

            int read_byte_cnt = mofs_read(fd, read_buffer, count);

            if (read_byte_cnt < 0) {
                Diagnose::PrintErrno("Read error");
                delete[] read_buffer;
                return -1;
            }

            cout << "Read " << read_byte_cnt << " byte(s), content : " << string(read_buffer, read_byte_cnt) << endl;
            delete[] read_buffer;
        }
        break;

        case FWRITE_MAP_VALUE: {
            int fd = -1, count = -1;
            input_stream >> fd >> count;
            if (fd == -1 || count == -1) {
                Diagnose::PrintError("Need more args.");
                return 0;
            }

            string write_data;
            getline(input_stream, write_data);

            int write_byte_cnt = mofs_write(fd, (void *) write_data.c_str(), count);

            if (write_byte_cnt < 0) {
                Diagnose::PrintErrno("Write error");
                return -1;
            }

            cout << "Write " << write_byte_cnt << " byte(s)." << endl;
        }
        break;

        case FLSEEK_MAP_VALUE: {
            int fd = 2147483647, offset = 2147483647;
            input_stream >> fd >> offset;
            if (fd == 2147483647 || offset == 2147483647) {
                Diagnose::PrintError("Need more args.");
                return 0;
            }

            string s_base;
            input_stream >> s_base;

            int base = -1;
            if (s_base == "SET") {
                base = SEEK_SET;
            }
            else if (s_base == "CUR") {
                base = SEEK_CUR;
            }
            else if (s_base == "END") {
                base = SEEK_END;
            }
            else {
                Diagnose::PrintErrno("Unrecognized token " + s_base);
                return 0;
            }

            int new_offset = mofs_lseek(fd, offset, base);

            cout << "Set pointer to " << new_offset << endl;
        }
        break;

        case FDELETE_MAP_VALUE: {
            string pathname;
            input_stream >> pathname;
            if (pathname.length() == 0) {
                Diagnose::PrintError("Need more args.");
                return 0;
            }

            if (mofs_unlink(pathname.c_str()) == -1) {
                Diagnose::PrintErrno("Cannot unlink file");
            }
        }
        break;

        case MVIN_MAP_VALUE: {
            string out_path, in_path;
            input_stream >> out_path >> in_path;
            if (out_path.length() == 0 || in_path.length() == 0) {
                Diagnose::PrintError("Need more args.");
                return 0;
            }

            FILE* out_file = fopen(out_path.c_str(), "rb");
            if (out_file == nullptr) {
                Diagnose::PrintErrno("Cannot open out file " + out_path);
                return 0;
            }

            // 从外部文件读，写入内部文件
            int fd = mofs_open(in_path.c_str(), MOFS_WRONLY | MOFS_CREAT, 0777);
            if (fd < 0) {
                Diagnose::PrintErrno("Cannot open / create in file " + in_path);
                return 0;
            }

            char trans_buffer[TRANS_BUFFER_SIZE];
            while (!feof(out_file)) {
                int read_byte_cnt = fread(trans_buffer, 1, TRANS_BUFFER_SIZE, out_file);

                if (read_byte_cnt < 0) {
                    Diagnose::PrintErrno("Out file read error");
                    return 0;
                }

                int write_byte_cnt = mofs_write(fd, trans_buffer, read_byte_cnt);
                if (read_byte_cnt != write_byte_cnt) {
                    Diagnose::PrintErrno("Transfer failed.");
                    return 0;
                }
            }

            fclose(out_file);

            if (-1 == mofs_close(fd)) {
                Diagnose::PrintErrno("Error occurred when closing");
                return 0;
            }
        }
        break;

        case MVOUT_MAP_VALUE: {
            string out_path, in_path;
            input_stream >> in_path >> out_path;
            if (out_path.length() == 0 || in_path.length() == 0) {
                Diagnose::PrintError("Need more args.");
                return 0;
            }


            int fd = mofs_open(in_path.c_str(), MOFS_RDONLY, 0);
            if (fd < 0) {
                Diagnose::PrintErrno("Cannot open in file " + in_path);
                return 0;
            }

            FILE* out_file = fopen(out_path.c_str(), "wb");
            if (out_file == nullptr) {
                Diagnose::PrintErrno("Cannot open out file " + out_path);
                return 0;
            }

            char trans_buffer[TRANS_BUFFER_SIZE];
            while (true) {
                int read_byte_cnt = mofs_read(fd, trans_buffer, TRANS_BUFFER_SIZE);

                if (read_byte_cnt < 0) {
                    Diagnose::PrintErrno("In file read error");
                    return 0;
                }

                if (read_byte_cnt == 0) {
                    break;
                }

                int write_byte_cnt = fwrite(trans_buffer, 1, read_byte_cnt, out_file);
                if (read_byte_cnt != write_byte_cnt) {
                    Diagnose::PrintErrno("Transfer failed.");
                    return 0;
                }
            }

            fclose(out_file);

            if (-1 == mofs_close(fd)) {
                Diagnose::PrintErrno("Error occurred when closing");
                return 0;
            }
        }
        break;

        case CHGUSR_MAP_VALUE: {
            int uid = -1, gid = -1;
            input_stream >> uid >> gid;
            if (uid == -1 || gid == -1) {
                Diagnose::PrintError("Need more args.");
                return 0;
            }

            User* newUser = User::ChangeUser(uid, gid);
            if (newUser == nullptr) {
                Diagnose::PrintErrno("Cannot change user");
                return 0;
            }

            User::userPtr = newUser;
            cli_header = "[User(" + to_string(User::userPtr->uid) + ", " + to_string(User::userPtr->gid) + ") ";
        }
        break;

        case CD_MAP_VALUE: {
            string pathname;
            input_stream >> pathname;
            if (pathname.length() == 0) {
                Diagnose::PrintError("Need more args.");
                return 0;
            }

            if (-1 == User::userPtr->ChangeDir(pathname.c_str())) {
                Diagnose::PrintErrno("Cannot change dir");
                return 0;
            }

            // 找到最小子目录
            int last_pos = -1;
            for (int i = pathname.length() - 1; i >= 0; --i) {
                if (pathname[i] == '/') {
                    last_pos = i;
                    break;
                }
            }
            if (last_pos < 0) {
                currentWorkDir = pathname;
            }
            else {
                currentWorkDir = pathname.substr(last_pos + 1, pathname.length() - last_pos);
            }
        }
        break;

        case LINK_MAP_VALUE: {
            string src_path, dst_path;
            input_stream >> src_path >> dst_path;
            if (src_path.length() == 0 || dst_path.length() == 0) {
                Diagnose::PrintError("Need more args.");
                return 0;
            }

            if (-1 == mofs_link(src_path.c_str(), dst_path.c_str())) {
                Diagnose::PrintErrno("Cannot create link");
                return 0;
            }
        }
        break;

        case EXIT_MAP_VALUE: {
            return -1;
        }

        case HELP_MAP_VALUE: {
            cout << "格式化:                                     mkfs [大小(MB): int] [最大inode数: int]\n"
                    "格式化:                                     fformat [大小(MB): int] [最大inode数: int]\n"
                    "展示目录下文件:                              ls [路径: str: 当前目录]\n"
                    "建立目录:                                   mkdir [路径目录名: str] [权限: oct_int]\n"
                    "创建文件(返回fd: int):                       fcreat [路径名: str] [权限: oct_int]\n"
                    "打开文件(返回fd: int):                       fopen [路径名: str] [要求: str] {权限: oct_int}     (要求：rw, r+, etc.)\n"
                    "关闭文件:                                   fclose [fd: int]\n"
                    "读取文件内容(返回读取字节数: int, 内容: str):   fread [fd: int] [读取字节数: int]\n"
                    "写入文件(返回实际读取的字节数: int):            fwrite [fd: int] [写入字节数: int] [写入内容(不允许有空格和换行): str]\n"
                    "设置读写指针(返回设置的指针位置: int):          flseek [fd: int] [偏移量: int] [起始位置: str]   (起始位置: SET / CUR / END)\n"
                    "删除(取消链接):                              fdelete [路径名: str]\n"
                    "拷贝入(返回拷贝字节数: int):                  mvin [外部路径名: str] [内部路径名: str]\n"
                    "拷贝出(返回拷贝字节数: int):                  mvout [内部路径名: str] [外部路径名: str]\n"
                    "切换用户:                                   chgusr [uid: int] [gid: int]\n"
                    "切换工作目录:                                cd [路径名: str]\n"
                    "创建硬链接:                                 link [源路径名: str] [目标路径名: str]\n"
                    "退出程序                                    exit\n"
                    "帮助与提示:                                 help" << endl;

            return 0;
        }

        default: {
            cout << "Unrecognized command." << endl;
            return -1;
        }
    }

    return 0;
}

/**
 * @brief 将tm结构体转换为string
 * @param t 传入的tm结构体
 * @return 转换结果
 */
string ConvertTimeToString(const struct tm& t) {
    char buffer[20];
    sprintf(buffer, "%04d-%02d-%02d %02d:%02d:%02d",
            t.tm_year + 1900, t.tm_mon + 1, t.tm_mday,
            t.tm_hour, t.tm_min, t.tm_sec);

    return string{buffer};
}

void print_list(const char* pathname) {
    FileStat fileStat;
    DirEntry entryBuffer;

    int dir_fd = mofs_open(pathname, MOFS_RDONLY | MOFS_DIRECTORY, 0);

    if (dir_fd < 0) {
        Diagnose::PrintErrno("Dir not find");
    }

    cout << std::left << setw(15) << "Permissions" << setw(8) << "Inode"
        << setw(8) << "User" << setw(8) << "Group"
        << setw(26) << "Access Time" << setw(26) << "Modify Time"
        << setw(12) << "Size(B)" << setw(8) << "Links" << "Name"
        << endl;
//    printf("Permissions\t\tInode\tAccess Time\t\tModify Time\t\tSize(B)\tLinks\t\tName\n");

    while (true) {
        int read_byte_cnt = mofs_read(dir_fd, &entryBuffer, sizeof(entryBuffer));
        if (read_byte_cnt == 0) {
            break;
        }

        if (read_byte_cnt != sizeof(entryBuffer)) {
            Diagnose::PrintErrno("Read dir error");
            break;
        }

        if (entryBuffer.m_ino <= 0) {
            continue;
        }

        if (-1 == mofs_inode_stat(entryBuffer.m_ino, &fileStat)) {
            Diagnose::PrintErrno("Get stat error.");
        }

        // 打印信息
        char authority_str[10] = "---------";
        for (int i = 0; i < 3; ++i) {
            if (((fileStat.st_mode >> (3 * i + 2)) & 1) == 1) {
                authority_str[8 - (3 * i + 2)] = 'r';
            }

            if (((fileStat.st_mode >> (3 * i + 1)) & 1) == 1) {
                authority_str[8 - (3 * i + 1)] = 'w';
            }

            if (((fileStat.st_mode >> (3 * i + 0)) & 1) == 1) {
                authority_str[8 - (3 * i + 0)] = 'x';
            }
        }

        // 转换时间
        time_t acc_tt = fileStat.st_atime;
        time_t mod_tt = fileStat.st_mtime;
        struct tm acc_time = *localtime((const time_t *const)(&acc_tt));
        struct tm mod_time = *localtime((const time_t *const)(&mod_tt));

        cout << std::left << setw(15) << authority_str << setw(8) << fileStat.st_ino
             << setw(8) << fileStat.st_uid << setw(8) << fileStat.st_gid
             << setw(26) << ConvertTimeToString(acc_time) << setw(26) << ConvertTimeToString(mod_time)
             << setw(12) << fileStat.st_size << setw(8) << fileStat.st_nlink << entryBuffer.m_name
             << endl;

//        printf("%s\t%d\t%04d-%02d-%02d %02d:%02d:%02d\t%04d-%02d-%02d %02d:%02d:%02d\t%d\t%d\t%s\n", authority_str,
//               fileStat.st_ino,
//               acc_time.tm_year + 1900, acc_time.tm_mon + 1, acc_time.tm_mday,
//               acc_time.tm_hour, acc_time.tm_min, acc_time.tm_sec,
//
//               mod_time.tm_year + 1900, mod_time.tm_mon + 1, mod_time.tm_mday,
//               mod_time.tm_hour, mod_time.tm_min, mod_time.tm_sec,
//
//               fileStat.st_size,
//               fileStat.st_nlink,
//               entryBuffer.m_name);
    }

    mofs_close(dir_fd);
}