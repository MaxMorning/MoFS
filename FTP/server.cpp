/**
 * @file server.c
 * @brief FTP服务器实现
 * @author Siim
 * @license BSD-3-Clause License
 * @mainpage https://github.com/Siim/ftp
 * @note 有大量改动以修正bug，调整排版和适配MoFS
*/

#include <cstring>
#include <cstdio>
#include <cctype>
#include "common.h"

#include "../utils/Diagnose.h"

int shutdown();

void sigint_handler(int sig) {
    Diagnose::PrintLog("Receive SIGINT, shutting down...");

    if (-1 == shutdown()) {
        Diagnose::PrintError("Shutdown error.");
        exit(-1);
    }
    exit(0);
}

/**
 * Sets up server and handles incoming connections
 * @param port Server port
 */
void server(int port) {
    int sock = create_socket(port);
    struct sockaddr_in client_address;
    unsigned int len = sizeof(client_address);
    int connection, pid, bytes_read;

    signal(SIGCHLD, SIG_IGN);
    signal(SIGINT, sigint_handler);

    Diagnose::PrintLog("Server established.");

    while (true) {
        connection = accept(sock, (struct sockaddr *) &client_address, &len);
        char buffer[BSIZE];
        Command *cmd = (Command *) malloc(sizeof(Command));
        State *state = (State *) malloc(sizeof(State));

        memset(buffer, 0, BSIZE);

        char welcome[BSIZE] = "220 ";
        if (strlen(welcome_message) < BSIZE - 4) {
            strcat(welcome, welcome_message);
        } else {
            strcat(welcome, "Welcome to nice FTP service.");
        }

        /* Write welcome message */
        strcat(welcome, "\n");
        write(connection, welcome, strlen(welcome));

        /* Read commands from client */
        while (true) {
            printf("Pre Read\n");
            bytes_read = read(connection, buffer, BSIZE);
            printf("Read return,");
            if (bytes_read <= 0) {
                break;
            }

            if (bytes_read <= BSIZE) {
                /* TODO: output this to log */
                buffer[BSIZE - 1] = '\0';
                printf("User %s sent command: %s\n", (state->username == nullptr) ? "unknown" : state->username, buffer);
                parse_command(buffer, cmd);
                state->connection = connection;

                /* Ignore non-ascii char. Ignores telnet command */
                if (buffer[0] <= 127 || buffer[0] >= 0) {
                    response(cmd, state);
                }
                memset(buffer, 0, BSIZE);
                memset(cmd, 0, sizeof(Command));
            }
            else {
                /* Read error */
                perror("server:read");
            }
        }

        free(state);
        free(cmd);

        close(connection);
        printf("Client disconnected.\n");
    }
}

/**
 * Creates socket on specified port and starts listening to this socket
 * @param port Listen on this port
 * @return int File descriptor for new socket
 */
int create_socket(int port) {
    int sock;
    int reuse = 1;

    /* Server address */
    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    memset(&(server_address.sin_addr), 0, sizeof(server_address.sin_addr));


    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        fprintf(stderr, "Cannot open socket");
        exit(EXIT_FAILURE);
    }

    /* Address can be reused instantly after program exits */
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof reuse);

    /* Bind socket to server address */
    if (bind(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        fprintf(stderr, "Cannot bind socket to address");
        exit(EXIT_FAILURE);
    }

    listen(sock, 32);
    return sock;
}

/**
 * Accept connection from client
 * @param socket Server listens this
 * @return int File descriptor to accepted connection
 */
int accept_connection(int socket) {
    int addrlen = 0;
    struct sockaddr_in client_address;
    addrlen = sizeof(client_address);
    return accept(socket, (struct sockaddr *) &client_address, (socklen_t *) (&addrlen));
}

/**
 * Get ip where client connected to
 * @param sock Commander socket connection
 * @param ip Result ip array (length must be 4 or greater)
 * result IP array e.g. {127,0,0,1}
 */
#define IP_BUFFER_SIZE 20

void getip(int sock, int *ip) {
    socklen_t addr_size = sizeof(struct sockaddr_in);
    struct sockaddr_in addr;
    getsockname(sock, (struct sockaddr *) &addr, &addr_size);

    char ip_addr_buffer[IP_BUFFER_SIZE];

    if (NULL == inet_ntop(
            AF_INET,
            &(addr.sin_addr),
            ip_addr_buffer,
            IP_BUFFER_SIZE
    )) {
        exit(-1);
    }

    sscanf(ip_addr_buffer, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
}

/**
 * Lookup enum value of string
 * @param cmd Command string 
 * @return Enum index if command found otherwise -1
 */

int lookup_cmd(char *cmd) {
    const int cmdlist_count = sizeof(cmdlist_str) / sizeof(char *);
    return lookup(cmd, cmdlist_str, cmdlist_count);
}

/**
 * General lookup for string arrays
 * It is suitable for smaller arrays, for bigger ones trie is better
 * data structure for instance.
 * @param needle String to lookup
 * @param haystack Strign array
 * @param count Size of haystack
 */
int lookup(char *needle, const char **haystack, int count) {
    int i;

    for (i = 0; i < count; i++) {
        if (strcmp(needle, haystack[i]) == 0)return i;
    }
    return -1;
}


/** 
 * Writes current state to client
 */
void write_state(State *state) {
    write(state->connection, state->message, strlen(state->message));

    printf("Server : %s\n", state->message);
}

/**
 * Generate random port for passive mode
 * @param state Client state
 */
void gen_port(Port *port) {
    srand(time(nullptr));
    port->p1 = 128 + (rand() % 64);
    port->p2 = rand() % 0xff;

}

/**
 * Parses FTP command string into struct
 * @param cmdstring Command string (from ftp client)
 * @param cmd Command struct
 */
void parse_command(char *cmdstring, Command *cmd) {
    sscanf(cmdstring, "%s %s", cmd->command, cmd->arg);
    for (int i = 0; i < strlen(cmd->command); ++i) {
        if (cmd->command[i] <= 'z' && cmd->command[i] >= 'a') {
            cmd->command[i] -= 'a' - 'A';
        }
    }
}
