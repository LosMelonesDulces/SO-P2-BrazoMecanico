#include <stdio.h>
#include "biblioteca.h"

int main() {
    // 1) Abrir el dispositivo que el driver expone
    BD fd = Biblioteca_Open("/dev/robotic_hand");
    if (fd < 0) {
        return 1;
    }

    // 2) Prueba de cada comando
    //printf("Moviendo motor1 hacia arriba (bit=1)...\n");
    /*if (Biblioteca_Motor1_Up(fd) < 0) {
        printf("Error al enviar comando Motor1_Up\n");
    }*/

    printf("Moviendo motor1 hacia abajo (bit=A)...\n");
    Biblioteca_Motor1_Down(fd);

    //printf("Moviendo motor2 hacia arriba (bit=3)...\n");
    //Biblioteca_Motor2_Up(fd);

    printf("Moviendo motor2 hacia abajo (bit=T)...\n");
    Biblioteca_Motor2_Down(fd);

    // 3) Cerrar
    Biblioteca_Close(fd);
    return 0;
}
