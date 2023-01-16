#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
#include<termios.h>
#include<sys/ioctl.h>
#include"terminal.h"
#include"common.h"

struct termios orig_termios;
extern struct msg_que recv_que;
extern struct msg_que send_que;

void orig_mode(){
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}
void raw_mode(){
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP| IXON);
    raw.c_oflag &= ~(OPOST);
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
int getcursor_pos(struct window win, char *esc_seq){
    char buf[32];
    int i = 0;
    if ((write(STDOUT_FILENO, "\x1b[6n", 4) != 4)) return -1;

    while (i < sizeof(buf)/sizeof(char)){
        if (read(STDIN_FILENO, &buf[i], 1) != 1) return -1;
        if (buf[i] == 'R') break;
        i++;
    }
    if (buf[0] != '\x1b' || buf[1] != '[') return -1;
    if (sscanf(&buf[2], "%d;%d", win.y, win.x) != 2) return -1;
    if (strcpy(esc_seq, buf) == NULL) return -1;
    return 0;
}

const char esc_seq[32] = "\x1b[H";

void print_message(int *io_lock, long unsigned int msg_num){
    extern struct window print_win;
    if (msg_num <= recv_que.m_num)
        if (recv_que.com_type == 'M'){
            while (*io_lock == WRITELOCK);
            *io_lock = READLOCK;
            write(STDOUT_FILENO, position_code[(print_win.y)*ws.ws_col+print_win.x], 8);
            write(STDOUT_FILENO, recv_que.buf[msg_num % 0xff], strlen(recv_que.buf[msg_num % 0xff]));
            write(STDOUT_FILENO, CURSOR_DOWN, 4);
            *io_lock = 0;
            print_win.y++;
            recv_que.com_type = 0;
            msg_num++;
        } else if (recv_que.com_type == 'P'){
            recv_que.com_type = 0;
        }
}

void *getkey(void *arg){ // get keyboard input from stdin
    char **io = (char**)arg;
    char c;
    int i = 0;
    while (read(STDIN_FILENO, &c, 1) == 1){
        **io = c;
        ++*io;
    }
}

void refresh_screen(){
    write(STDOUT_FILENO, "\x1b[2J", 4);
}