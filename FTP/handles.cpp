/**
 * @file handles.c
 * @brief FTP处理函数实现
 * @author Siim
 * @license BSD-3-Clause License
 * @mainpage https://github.com/Siim/ftp
 * @note 有所改动以修正bug，调整排版和适配macOS 和 MoFS
*/
#include <cerrno>
#include <string>

#include "common.h"
#include "../include/User.h"
#include "../utils/Diagnose.h"
#include "../include/Primitive.h"

#define BUF_SIZE 8192

int send_file(int out_fd, int in_fd, size_t count) {
    char buf[BUF_SIZE];
    size_t toRead, numRead, numSent, totSent;

    mofs_lseek(in_fd, 0, SEEK_SET);

    int send_byte_sum = 0;
    while (true) {
        int read_byte_cnt = mofs_read(in_fd, buf, BUF_SIZE);
        if (read_byte_cnt <= 0) {
            break;
        }

        int send_byte_cnt = send(out_fd, buf, read_byte_cnt, 0);
        if (send_byte_cnt < 0) {
            perror("Send error");
            break;
        }
        send_byte_sum += send_byte_cnt;
        if (send_byte_cnt == 0) {
            break;
        }
    }
    return send_byte_sum;
}


/**
 * Generates response message for client
 * @param cmd Current command
 * @param state Current connection state
 */
void response(Command *cmd, State *state) {
    switch (lookup_cmd(cmd->command)) {
        case USER:
            ftp_user(cmd, state);
            break;
        case PASS:
            ftp_pass(cmd, state);
            break;
        case PASV:
            ftp_pasv(cmd, state);
            break;
        case LIST:
            ftp_list(cmd, state);
            break;
        case CWD:
            ftp_cwd(cmd, state);
            break;
        case PWD:
            ftp_pwd(cmd, state);
            break;
        case MKD:
            ftp_mkd(cmd, state);
            break;
        case RMD:
            ftp_rmd(cmd, state);
            break;
        case RETR:
            ftp_retr(cmd, state);
            break;
        case STOR:
            ftp_stor(cmd, state);
            break;
        case DELE:
            ftp_dele(cmd, state);
            break;
        case SIZE:
            ftp_size(cmd, state);
            break;
        case ABOR:
            ftp_abor(state);
            break;
        case QUIT:
            ftp_quit(state);
            break;
        case TYPE:
            ftp_type(cmd, state);
            break;
        case NOOP:
            if (state->logged_in) {
                state->message = "200 Nice to NOOP you!\n";
            } else {
                state->message = "530 NOOB hehe.\n";
            }
            write_state(state);
            break;

        case OPTS:
            state->message = "200 OPTS UTF8 is set to ON.\n";
            write_state(state);
            break;
        default:
            state->message = "500 Unknown command\n";
            write_state(state);
            break;
    }
}

/**
 * Handle USER command
 * @param cmd Command with args
 * @param state Current client connection state
 */
void ftp_user(Command *cmd, State *state) {
    const int total_usernames = sizeof(usernames) / sizeof(char *);
    if (lookup(cmd->arg, usernames, total_usernames) >= 0) {
        state->username = (char *) malloc(32);
        memset(state->username, 0, 32);
        strcpy(state->username, cmd->arg);
        state->username_ok = 1;
        state->message = "331 User name okay, need password\n";
    } else {
        state->message = "530 Invalid username\n";
    }
    write_state(state);
}

/** PASS command */
void ftp_pass(Command *cmd, State *state) {
    if (state->username_ok == 1) {
        state->logged_in = 1;
        state->message = "230 Login successful\n";
    } else {
        state->message = "500 Invalid username or password\n";
    }
    write_state(state);
}

/** PASV command */
void ftp_pasv(Command *cmd, State *state) {
    if (state->logged_in) {
        int ip[4];
        char buff[255];
        char *response = "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\n";
        Port *port = (Port *) malloc(sizeof(Port));
        gen_port(port);
        getip(state->connection, ip);

        /* Close previous passive socket? */
        close(state->sock_pasv);

        /* Start listening here, but don't accept the connection */
        state->sock_pasv = create_socket((256 * port->p1) + port->p2);
        printf("port: %d\n", 256 * port->p1 + port->p2);
        sprintf(buff, response, ip[0], ip[1], ip[2], ip[3], port->p1, port->p2);
        state->message = buff;
        state->mode = SERVER;
        puts(state->message);

    } else {
        state->message = "530 Please login with USER and PASS.\n";
        printf("%s", state->message);
    }
    printf("Data to be send : %s\n", state->message);
    write_state(state);
}

/**
 * @brief 将tm结构体转换为string
 * @param t 传入的tm结构体
 * @return 转换结果
 */
string ConvertTimeToString(const struct tm &t);

/** LIST command */
// cmd->arg 是要列出的目录
void ftp_list(Command *cmd, State *state) {
    if (state->logged_in == 1) {
        int connection;
        int dir_fd;

        if (cmd->arg[0] == '-') {
            dir_fd = mofs_open(User::userPtr->currentWorkPath, MOFS_RDONLY | MOFS_DIRECTORY, 0);
            Diagnose::PrintLog("Current work dir " + string(User::userPtr->currentWorkPath) + ", " +
                               to_string(User::userPtr->userOpenFileTable[dir_fd].f_inode->i_number));
        } else {
            dir_fd = mofs_open(cmd->arg, MOFS_RDONLY | MOFS_DIRECTORY, 0);
        }
        if (dir_fd == -1) {
            state->message = "550 Failed to open directory.\n";
        } else {
            if (state->mode == SERVER) {
                connection = accept_connection(state->sock_pasv);
                state->message = "150 Here comes the directory listing.\n";
                puts(state->message);
                write_state(state);

                FileStat fileStat;
                DirEntry entryBuffer;

                mofs_lseek(dir_fd, 0, SEEK_SET);

                while (true) {
                    int read_byte_cnt = mofs_read(dir_fd, &entryBuffer, sizeof(entryBuffer));
                    Diagnose::PrintLog("Dir file read " + to_string(read_byte_cnt) + " byte(s).");
                    if (read_byte_cnt == 0) {
                        break;
                    }

                    if (read_byte_cnt != sizeof(entryBuffer)) {
                        Diagnose::PrintErrno("Read dir error");
                        state->message = "550 Failed to open directory.\n";
                        break;
                    }
                    if (entryBuffer.m_ino <= 0) {
                        continue;
                    }

                    if (-1 == mofs_inode_stat(entryBuffer.m_ino, &fileStat)) {
                        Diagnose::PrintErrno("Get stat error.");
                    }

                    // 权限信息
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
                    struct tm acc_time = *localtime((const time_t *const) (&acc_tt));
                    struct tm mod_time = *localtime((const time_t *const) (&mod_tt));

                    // 检查是否是目录
                    OpenFile openFile;
                    if (-1 == OpenFile::OpenFileFactory(openFile, entryBuffer.m_ino, User::userPtr->uid, User::userPtr->gid, FileFlags::MOFS_READ)) {
                        // 无法加载
                        continue;
                    }

                    bool isDir = openFile.IsDirFile();
                    openFile.Close(false);

                    char time_buffer[32];
                    strftime(time_buffer, 32, "%b %d %H:%M", &mod_time);

                    char buffer[1024];
                    sprintf(buffer,
                            "%c%s %5d %4d %4d %8d %s %s\r\n",
                            isDir ? 'd' : '-',
                            authority_str,
                            fileStat.st_nlink,
                            fileStat.st_uid,
                            fileStat.st_gid,
                            fileStat.st_size,
                            time_buffer,
                            entryBuffer.m_name);

                    Diagnose::PrintLog("Entry data : " + string{buffer});
                    int expected_length = strlen(buffer) + 1;
                    int send_byte_cnt = send(connection, buffer, expected_length, 0);
                    if (send_byte_cnt != expected_length) {
                        state->message = "550 unknown error.\n";
                        write_state(state);
                        close(connection);
                    }
                }

                state->message = "226 Directory send OK.\n";
                state->mode = NORMAL;
                close(connection);

            } else if (state->mode == CLIENT) {
                state->message = "502 Command not implemented.\n";
            } else {
                state->message = "425 Use PASV or PORT first.\n";
            }

            mofs_close(dir_fd);
        }
    } else {
        state->message = "530 Please login with USER and PASS.\n";
    }
    state->mode = NORMAL;
    write_state(state);
}


/** QUIT command */
void ftp_quit(State *state) {
    state->message = "221 Goodbye, friend. I never thought I'd die like this.\n";
    write_state(state);
    close(state->connection);
    exit(0);
}

/** PWD command */
void ftp_pwd(Command *cmd, State *state) {
    if (state->logged_in) {
        char cwd[BSIZE];
        char result[BSIZE];
        memset(result, 0, BSIZE);
        if (getcwd(cwd, BSIZE - 8) !=
            nullptr) {    //Making sure the length of cwd and additional text don't exceed BSIZE
            strcat(result, "257 \"");        //5 characters
            strcat(result, User::userPtr->currentWorkPath);        //strlen(cwd)
            strcat(result, "\"\n");        //2 characters + 0 byte
            state->message = result;
        } else {
            state->message = "550 Failed to get pwd.\n";
        }
        write_state(state);
    }
}

/** CWD command */
void ftp_cwd(Command *cmd, State *state) {
    if (state->logged_in) {
        if (User::userPtr->ChangeDir(cmd->arg) == 0) {
            state->message = "250 Directory successfully changed.\n";
        } else {
            state->message = "550 Failed to change directory.\n";
        }
    } else {
        state->message = "500 Login with USER and PASS.\n";
    }
    write_state(state);

}

/** 
 * MKD command 
 * TODO: full path directory creation
 */
void ftp_mkd(Command *cmd, State *state) {
    if (state->logged_in) {
        char cwd[BSIZE];
        char res[2 * BSIZE +
                 32]; //Increasing buffer size to accomodate all usecases, doubling and adding 32 extra characters for the sprintf
        memset(cwd, 0, BSIZE);
        memset(res, 0, 2 * BSIZE + 32);
        getcwd(cwd, BSIZE);

        /* TODO: check if directory already exists with chdir? */

        /* Absolute path */
        if (cmd->arg[0] == '/') {
            if (mofs_mkdir(cmd->arg, 0777) == 0) {
                strcat(res, "257 \"");
                strcat(res, cmd->arg);
                strcat(res, "\" new directory created.\n");
                state->message = res;
            } else {
                state->message = "550 Failed to create directory. Check path or permissions.\n";
            }
        }
            /* Relative path */
        else {
            if (mofs_mkdir(cmd->arg, 0777) == 0) {
                sprintf(res, "257 \"%s/%s\" new directory created.\n", cwd, cmd->arg); //32 additional characters
                state->message = res;
            } else {
                state->message = "550 Failed to create directory.\n";
            }
        }
    } else {
        state->message = "500 Good news, everyone! There's a report on TV with some very bad news!\n";
    }
    write_state(state);
}

/** RETR command */
void ftp_retr(Command *cmd, State *state) {

    int connection;
    int fd;

    int sent_total = 0;
    if (state->logged_in) {

        /* Passive mode */
        if (state->mode == SERVER) {
            fd = mofs_open(cmd->arg, MOFS_RDONLY, 0);
            if (fd >= 0) {
                state->message = "150 Opening BINARY mode data connection.\n";

                write_state(state);

                int file_size = User::userPtr->userOpenFileTable[fd].f_inode->i_size;

                connection = accept_connection(state->sock_pasv);
                sent_total = send_file(connection, fd, file_size);
                if (sent_total) {

                    if (sent_total != file_size) {
                        perror("ftp_retr:send_file");
                        exit(EXIT_SUCCESS);
                    }

                    state->message = "226 File send OK.\n";
                } else {
                    state->message = "550 Failed to read file.\n";
                }

                mofs_close(fd);
                close(connection);
            } else {
                state->message = "550 Failed to get file\n";
            }
        } else {
            state->message = "550 Please use PASV instead of PORT.\n";
        }
    } else {
        state->message = "530 Please login with USER and PASS.\n";
    }


    write_state(state);
}

/** Handle STOR command. TODO: check permissions. */
void ftp_stor(Command *cmd, State *state) {
    int connection, fd;
    const int buff_size = 8192;

    fd = mofs_open(cmd->arg, MOFS_WRONLY | MOFS_CREAT, 0777);

    if (fd == -1) {
        /* TODO: write status message here! */
        perror("ftp_stor:fopen");
        state->message = "550 No such file or directory.\n";
    } else if (state->logged_in) {
        if (state->mode != SERVER) {
            state->message = "550 Please use PASV instead of PORT.\n";
        }
            /* Passive mode */
        else {
            connection = accept_connection(state->sock_pasv);

            state->message = "125 Data connection already open; transfer starting.\n";
            write_state(state);

            // 将connection中读到的数据写入fd中。原作者使用了splice函数。这是一个Linux独有的函数，需要将其替换掉。
            /* Using splice function for file receiving.
             * The splice() system call first appeared in Linux 2.6.17.
             */

//        while ((res = splice(connection, 0, pipefd[1], NULL, buff_size, SPLICE_F_MORE | SPLICE_F_MOVE))>0){
//          splice(pipefd[0], NULL, fd, 0, buff_size, SPLICE_F_MORE | SPLICE_F_MOVE);
//        }
            char transfer_buffer[buff_size];
            long write_byte_cnt = 1;
            int total_trans_byte = 0;
            errno = 0;
            while (true) {
                long recv_byte_cnt = recv(connection, transfer_buffer, buff_size, 0);
                printf("Recv %ld byte(s)\n", recv_byte_cnt);
                if (recv_byte_cnt <= 0) {
                    // 读取完成
                    break;
                }

                write_byte_cnt = mofs_write(fd, transfer_buffer, recv_byte_cnt);
                if (write_byte_cnt != recv_byte_cnt) {
                    // 没有全部写入，有问题
                    break;
                }

                total_trans_byte += write_byte_cnt;
            }

            /* Internal error */
            if (write_byte_cnt <= -1) {
                perror("ftp_stor: transfer");
                exit(EXIT_SUCCESS);
            } else {
                printf("Transfer %d byte(s).\n", total_trans_byte);
                state->message = "226 File send OK.\n";
            }
            close(connection);
            mofs_close(fd);
        }
    } else {
        state->message = "530 Please login with USER and PASS.\n";
    }
    write_state(state);
}

/** ABOR command */
void ftp_abor(State *state) {
    if (state->logged_in) {
        state->message = "226 Closing data connection.\n";
        state->message = "225 Data connection open; no transfer in progress.\n";
    } else {
        state->message = "530 Please login with USER and PASS.\n";
    }
    write_state(state);

}

/** 
 * Handle TYPE command.
 * BINARY only at the moment.
 */
void ftp_type(Command *cmd, State *state) {
    if (state->logged_in) {
        if (cmd->arg[0] == 'I') {
            state->message = "200 Switching to Binary mode.\n";
        } else if (cmd->arg[0] == 'A') {

            /* Type A must be always accepted according to RFC */
            state->message = "200 Switching to ASCII mode.\n";
        } else {
            state->message = "504 Command not implemented for that parameter.\n";
        }
    } else {
        state->message = "530 Please login with USER and PASS.\n";
    }
    write_state(state);
}

/** Handle DELE command */
void ftp_dele(Command *cmd, State *state) {
    if (state->logged_in) {
        if (mofs_unlink(cmd->arg) == -1) {
            state->message = "550 File unavailable.\n";
        } else {
            state->message = "250 Requested file action okay, completed.\n";
        }
    } else {
        state->message = "530 Please login with USER and PASS.\n";
    }
    write_state(state);
}

/** Handle RMD */
void ftp_rmd(Command *cmd, State *state) {
    if (!state->logged_in) {
        state->message = "530 Please login first.\n";
    } else {
        if (unlink(cmd->arg) == 0) {
            state->message = "250 Requested file action okay, completed.\n";
        } else {
            state->message = "550 Cannot delete directory.\n";
        }
    }
    write_state(state);

}

/** Handle SIZE (RFC 3659) */
void ftp_size(Command *cmd, State *state) {
    if (state->logged_in) {
        FileStat fileStat;
        char filesize[128];
        memset(filesize, 0, 128);
        /* Success */
        if (mofs_stat(cmd->arg, &fileStat) == 0) {
            sprintf(filesize, "213 %d\n", fileStat.st_size);
            state->message = filesize;
        } else {
            state->message = "550 Could not get file size.\n";
        }
    } else {
        state->message = "530 Please login with USER and PASS.\n";
    }

    write_state(state);
}
