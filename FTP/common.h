/**
 * @file common.h
 * @brief FTP通用头文件
 * @author Siim, 韩孟霖
 * @license BSD-3-Clause License
 * @mainpage https://github.com/Siim/ftp
 * @note 有大量改动以修正bug，调整排版和适配MoFS
 */

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <csignal>
#include <sys/types.h>
#include <sys/socket.h>
#include <pwd.h>
#include <netinet/in.h>
#include <ctime>
#include <dirent.h>
#include <arpa/inet.h>

#ifndef BSIZE
#define BSIZE 1024
#endif

typedef struct Port {
    int p1;
    int p2;
} Port;

typedef struct State {
    /* Connection mode: NORMAL, SERVER, CLIENT */
    int mode;

    /* Is user logged in? */
    int logged_in;

    /* Is this username allowed? */
    int username_ok;
    char *username;

    /* Response message to client e.g. 220 Welcome */
    char *message;

    /* Commander connection */
    int connection;

    /* Socket for passive connection (must be accepted later) */
    int sock_pasv;

    /* Transfer process id */
    int tr_pid;

} State;


/* Command struct */
typedef struct Command {
    char command[5];
    char arg[BSIZE];
} Command;

/**
 * Connection mode
 * NORMAL - normal connection mode, nothing to transfer
 * SERVER - passive connection (PASV command), server listens
 * CLIENT - server connects to client (PORT command)
 */
typedef enum conn_mode {
    NORMAL, SERVER, CLIENT
} conn_mode;

/* Commands enumeration */
typedef enum cmdlist {
    ABOR, CWD, DELE, LIST, MDTM, MKD, NLST, PASS, PASV,
    PORT, PWD, QUIT, RETR, RMD, RNFR, RNTO, SITE, SIZE,
    STOR, TYPE, USER, NOOP, OPTS
} cmdlist;

/* String mappings for cmdlist */
static const char *cmdlist_str[] =
        {
                "ABOR", "CWD", "DELE", "LIST", "MDTM", "MKD", "NLST", "PASS", "PASV",
                "PORT", "PWD", "QUIT", "RETR", "RMD", "RNFR", "RNTO", "SITE", "SIZE",
                "STOR", "TYPE", "USER", "NOOP", "OPTS"
        };

/* Valid usernames for anonymous ftp */
static const char *usernames[] =
        {
                "ftp", "anonymous", "public", "anon", "test", "foo", "siim"
        };

/* Welcome message */
static char *welcome_message = "A very warm welcome!";

/* Server functions */
void server(int port);

void gen_port(Port *);

void parse_command(char *, Command *);

int create_socket(int port);

void write_state(State *);

int accept_connection(int);

void getip(int sock, int *ip);

int lookup_cmd(char *cmd);

int lookup(char *needle, const char **haystack, int count);

/* Commands handle functions*/
void response(Command *, State *);

void ftp_user(Command *, State *);

void ftp_pass(Command *, State *);

void ftp_pwd(Command *, State *);

void ftp_cwd(Command *, State *);

void ftp_mkd(Command *, State *);

void ftp_rmd(Command *, State *);

void ftp_pasv(Command *, State *);

void ftp_list(Command *, State *);

void ftp_retr(Command *, State *);

void ftp_stor(Command *, State *);

void ftp_dele(Command *, State *);

void ftp_size(Command *, State *);

void ftp_quit(State *);

void ftp_type(Command *, State *);

void ftp_abor(State *);

void ftp_rnfr(Command *, State *);

void ftp_rnto(Command *, State *);
