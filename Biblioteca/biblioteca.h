#ifndef BIBLIOTECA_H
#define BIBLIOTECA_H

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int BD;

// Abre el dispositivo y retorna descriptor, o -1 en error
BD Biblioteca_Open(const char *device_path);

// Cierra descriptor, 0 éxito o -1 en error
int Biblioteca_Close(BD fd);

// Comandos de motor1 y motor2 (dos dedos)
int Biblioteca_Motor1_Down(BD fd);  // Correspondiente a 'A'
//int Biblioteca_Motor1_Up(BD fd);    // Correspondiente a '1'
int Biblioteca_Motor2_Down(BD fd);  // Correspondiente a 'T'
//int Biblioteca_Motor2_Up(BD fd);    // Correspondiente a '3'

// Función genérica para enviar un solo byte al driver
typedef char Cmd;
int Biblioteca_SendCommand(BD fd, Cmd cmd);

#ifdef __cplusplus
}
#endif

#endif // BIBLIOTECA_H
