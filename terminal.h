#ifndef TERMINAL_H
#define TERMINAL_H


void orig_mode();
void raw_mode();
void print_message(int *io_lock, long unsigned int msg_num);
void *getkey(void *arg);

#endif