#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<pthread.h>
#include<sys/ioctl.h>
#include"common.h"
#include"net.h"
#include"terminal.h"
#include"file.h"

struct ctx ctx = {0};

struct msg_que recv_que = {0}; // que for recieved messages
struct msg_que send_que = {0}; // que for sending messages

struct window win; // store cursor position (keyboard input)
struct window print_win;
struct winsize ws;

int sock_fd;

struct com com_list[] = {
    {"connect", CON},
    {"user", USR},
    {"nick", NICK},
    {"join", JOIN}
};

struct com recv_com_list[] = { // list of potential commands that could be sent from server.
    {"PING", 'P'},
};

int io_lock = 0; //Similair to mutex

char **position_code; // stores the ANSI escape codes for every position on the terminal

int format_message(char *raw_message){ // format message string to IRC specifications
    bzero(send_que.buf[(send_que.m_num) % 0xff], BUFLEN * sizeof(unsigned char));
    snprintf(send_que.buf[(send_que.m_num) % 0xff], sizeof(unsigned char)*(strlen(raw_message)+4), "%s\r\n", raw_message);
}

int getcom(char *buf, void *arg){
    char *loc;
    unsigned char temp[BUFLEN] = {0}; //store plaintext messages for sending
    char prefix[0x40] = {0};
    bool is_command = false;
    int i = 0; // size of prefix and leading spaces
    int n = 0;
    int k = 0;
    int r = 0;
        
    while (buf[i] == ' ')
        i++;

    if (buf[i] == '/' && buf[i+1] != 0){
        is_command = true;
        i++;
    }

    while (buf[i] != ' ' && buf[i] != 0){
        prefix[n] = buf[i];
        i++;
        n++;
    }
    n = 0;
        if (is_command == true){
            for (int j = 0; j < sizeof(com_list)/sizeof(struct com); j++){
                if (strcmp(prefix, com_list[j].name) == 0){
                    switch (com_list[j].def){
                        case CON: // if /connect command is passed
                            io_lock = READLOCK;
                            if (strlen(ctx.username) == 0){
                                printf("Error: You need a username to connect. Please create a configuration file or select one first with /user <username>.\r\n");
                                io_lock = 0;
                                break;
                            }       
                            n = i;
                            k = 0;
                            while (buf[n] == ' ' && buf[n] != 0)
                                n++;

                            while (buf[n] != ' ' && buf[n] != 0 && buf[n] != '\n' && buf[n] != '\r'){
                                ctx.serv_addr[k] = buf[n];
                                n++;
                                k++;        
                            }
                            k = 0;
                            while (buf[n] == ' ' && buf[n] != 0)
                                n++;

                            while (buf[n] != ' ' && buf[n] != 0 && buf[n] != '\n' && buf[n] != '\r'){
                                ctx.serv_port[k] = buf[n];
                                n++;
                                k++;
                            }
                            k = 0;
                            sock_fd = client_connect(ctx.serv_addr, atoi(ctx.serv_port));   
                            if (sock_fd == -1){
                                return -1;
                            } 
                            pthread_create((pthread_t*)arg, NULL, recv_message, NULL); // create thread to receive messages    
                            usleep(100);                            
                            snprintf(send_que.buf[send_que.m_num % 0xff], (sizeof(ctx.nickname) + 10)*sizeof(unsigned char), "NICK %s\r\n", ctx.nickname);
                            send_message(sock_fd);

                            snprintf(send_que.buf[send_que.m_num % 0xff], (sizeof(ctx.username) + 21), "USER guest 0 * :%s\r\n", ctx.username);
                            send_message(sock_fd);

                            ctx.is_connected = true;

                            io_lock = 0;

                            bzero(temp, BUFLEN);
                            return;

                        case USR: // if /user command is passed
                            n = i;
                            k = 0;
                            while (buf[n] == ' ' && buf[n] != 0)
                                n++;

                            while (buf[n] != ' ' && buf[n] != 0 && buf[n] != '\n' && buf[n] != '\r'){
                                ctx.username[k] = buf[n];
                                n++;
                                k++;
                            }
                            k = 0;
                            return;

                        case NICK: // if /nick command is passed
                            n = i;
                            k = 0;
                            while (buf[n] == ' ' && buf[n] != 0)
                                n++;

                            while (buf[n] != ' ' && buf[n] != 0 && buf[n] != '\n' && buf[n] != '\r'){
                                ctx.nickname[k] = buf[n];
                                n++;
                                k++;
                            }
                            k = 0;
                            return;

                        case JOIN:
                            n = i;
                            k = 0;
                            while (buf[n] == ' ' && buf[n] != 0)
                                n++;
                            while (buf[n] != ' ' && buf[n] != 0 && buf[n] != '\n' && buf[n] != '\r'){
                                ctx.channel[k] = buf[n];
                                n++;
                                k++;
                            }
                            k = 0;
                            snprintf(send_que.buf[send_que.m_num % 0xff], strlen(ctx.channel) + 7, "JOIN %s\r\n", ctx.channel);
                            send_message(sock_fd);
                            return;

                        default:
                            break;
               
                    }
                }
            }
        }else{  
            if (ctx.is_connected == true){
                for (n = 0; n < strlen(buf); n++){
                    if (buf[n] != '\n' && buf[n] != '\r')
                        temp[n] = buf[n];
                }
                snprintf(send_que.buf[send_que.m_num % 0xff], strlen(temp) + strlen(ctx.channel) + 14, "PRIVMSG %s :%s\r\n", ctx.channel, temp);
                send_message(sock_fd);
                return;
            }
        }

}

void *parse_message(void *arg){ // Figure out what kind of command is being recieved
    char *loc;
    char buf[64] = {0};
    recv_que.com_type = 0;
    int temp_type;
    int count = 0;
    while (1){
        if (count < recv_que.m_num){
        for (int i = 0; i < sizeof(recv_com_list)/(sizeof(struct com)); i++){
        if ((loc = strstr(recv_que.buf[count % 0xff], recv_com_list[i].name)) != NULL){
            switch (recv_com_list[i].def){ 
                case 'P': // server sends "PING" request
                    
                    strncpy(buf, loc+strlen("PONG")+2, 16);
                    temp_type = 'P';
                    snprintf(send_que.buf[count % 0xff], BUFLEN-1, "PONG :%s\r\n", buf);
                    send_message(sock_fd);
                    break;
            }
            recv_que.com_type = temp_type;
            break;
        }else{ // recieve standard message
            recv_que.com_type = 'M';
            print_message(&io_lock, count);
            write(STDOUT_FILENO, position_code[(ws.ws_col*(ws.ws_row-1)+win.x+1)], strlen(position_code[(ws.ws_col*(ws.ws_row-1)+win.x+1)]));
            break;
        }
        }
        count++;
        }
    }
}
int putcursor_pos(struct winsize ws, char *esc_seq){ // write current cursor position to buffer as ANSI escape code.
    snprintf(esc_seq, 32, "\x1b[%d;1H", ws.ws_row);
    return 0;
}

int main(int argc, char **argv){
    pthread_t input_thread;
    pthread_t print_thread;
    pthread_t recv_thread; 
    
    char write_pos[32];

    char *io = malloc(sizeof(char)*64*BUFLEN);

    FILE* fp;
    
    if ((fp = open_file("config", "r+")) == NULL){
        if ((fp = open_file("config", "w+")) == NULL){
            //handle error
        }
        // parse file, set user, nick attributes
    }

    raw_mode(); //convert terminal to raw mode

    if (argc != 3){
        printf("Usage: vanillairc <server> <port>\n");
        return -1;
    }

    ioctl(0, TIOCGWINSZ, &ws);
    
    position_code = (char**)malloc(sizeof(char*) * ws.ws_row * ws.ws_col);

    for (int i = 0; i < ws.ws_row*ws.ws_col; i++){
        position_code[i] = (char*)malloc(sizeof(char)*16);
    }

    for (int r = 0; r < ws.ws_row; r++){ // function to map terminal positions to position_code buffer as escape codes
        for (int i = 0; i < ws.ws_col; i++){
            snprintf(position_code[(ws.ws_col*r)+i], 16, "\x1b[%d;%dH", r, i); 
        }
    }
    refresh_screen();
    write(STDOUT_FILENO, "\x1b[H", 3);

    win.x = 0;
    win.y = 0;
        
    putcursor_pos(ws, write_pos);

    char *p = io;

    pthread_create(&input_thread, NULL, getkey, (void*)(&io)); // thread for keyboard input
    pthread_create(&print_thread, NULL, parse_message, NULL); // thread for parsing incoming messages
    

    int i = 0;
    
    char temp[BUFLEN] = {0};
    
    while (1){
        if (p < io){
            win.x++;
            while (io_lock == READLOCK);
            io_lock = WRITELOCK;
            temp[i] = *p;
            i++;
            write(STDOUT_FILENO, p, 1);
            if (*p == '\r' || *p == '\n'){
                win.x = 0;
                write(STDOUT_FILENO, "\x1b[2K", 5);
                if (getcom(temp, (void*)&recv_thread) == -1){
                    return -1;
                }
                bzero(temp, BUFLEN);
                i = 0;
            }  
            if (*p == BACKSPACE){
                write(STDOUT_FILENO, "\x1b[D", 4);
                write(STDOUT_FILENO, " ", 1);
                write(STDOUT_FILENO, "\x1b[D", 4);
                if (i > 0){
                    temp[i] = 0;
                    i -= 1;
                }
                win.x--;
            }
            p++;
            io_lock = 0;
        } else {
            io_lock = 0;
        }
    }

    pthread_exit(NULL);
    orig_mode(); //terminal back to cooked mode
   
}
