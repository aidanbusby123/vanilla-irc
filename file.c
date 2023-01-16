#include"file.h"

FILE* open_file(char *fname, char *mode){
    FILE* fp;
    fp = fopen(fname, mode);
    if (fp == NULL)
        return NULL;
    return fp;
}