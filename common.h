#ifndef COMMON_H
#define COMMON_H

#include<sys/ioctl.h>

#define BUFLEN 512

#define CTRL_KEY(k) ((k) & 0x1f)

#define CURSOR_HOME "\x1b[H"
#define CURSOR_DOWN "\x1b[1B"
#define CURSOR_LEFT "\x1b[1D"
#define CURSOR_RIGHT "\x1b[1C"
#define ERASE_LINE "\x1b[2K"

#define READLOCK 1
#define WRITELOCK 2


#define BACKSPACE 127

//command types

#define CON 1
#define USR 2
#define NICK 3
#define JOIN 4

//


typedef enum {false, true} bool;

struct ctx{
    bool is_connected;
    char serv_addr[64];
    char serv_port[16];

    char username[64];
    char nickname[64];
    char password[64];
    char channel[64];
};

struct msg_que{
    unsigned char buf[0xff][BUFLEN];
    long long unsigned int m_num;
    int com_type;
};

struct window{
    int x;
    int y;
};

struct com{
    char *name;
    int def;
}; // struct to hold list of potential commands that can be recieved

extern struct window print_win;
extern struct winsize ws;

extern struct msg_que raw_message[2];


extern char **position_code; // contains escape codes for every terminal position

int getcom(char *buf, void *arg);

#endif