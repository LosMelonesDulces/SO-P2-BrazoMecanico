#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include "biblioteca.h"

BD Biblioteca_Open(const char *device_path) {
    BD fd = open(device_path, O_RDWR);
    if (fd < 0) perror("Biblioteca_Open");
    return fd;
}

int Biblioteca_Close(BD fd) {
    if (close(fd) < 0) {
        perror("Biblioteca_Close");
        return -1;
    }
    return 0;
}

int Biblioteca_SendCommand(BD fd, Cmd cmd) {
    ssize_t n = write(fd, &cmd, 1);
    if (n < 0) {
        perror("Biblioteca_SendCommand");
        return -1;
    }
    if (n != 1) {
        fprintf(stderr, "Biblioteca_SendCommand: enviado != 1 byte\n");
        return -1;
    }
    return 0;
}

int Biblioteca_Motor1_Down(BD fd) { return Biblioteca_SendCommand(fd, 'A'); }
//int Biblioteca_Motor1_Up(BD fd)   { return Biblioteca_SendCommand(fd, '1'); }
int Biblioteca_Motor2_Down(BD fd) { return Biblioteca_SendCommand(fd, 'T'); }
//int Biblioteca_Motor2_Up(BD fd)   { return Biblioteca_SendCommand(fd, '3'); }
