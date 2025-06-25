#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include "node_manager.h"
#include <mpi.h>

void die_with_error(const char *message) {
    perror(message);
    exit(EXIT_FAILURE);
}

int recv_all(int socket, void *buffer, size_t length) {
    unsigned char *ptr = (unsigned char*) buffer;
    while (length > 0) {
        int i = recv(socket, ptr, length, 0);
        if (i < 1) return -1;
        ptr += i;
        length -= i;
    }
    return 0;
}

void handle_client(int client_socket, const char *key) {
    // 1. Recibir el tamaño del archivo
    uint64_t net_size, file_size;
    if (recv(client_socket, &net_size, sizeof(net_size), 0) != sizeof(net_size)) {
        fprintf(stderr, "[HANDLER] Error al recibir el tamaño.\n");
        close(client_socket);
        return;
    }
    file_size = be64toh(net_size);
    printf("[HANDLER] Se recibirán %zu bytes.\n", file_size);

    // 2. Alojar memoria y recibir el archivo cifrado
    unsigned char *buffer_cifrado = malloc(file_size);
    if (!buffer_cifrado) {
        fprintf(stderr, "[HANDLER] No se pudo alojar memoria.\n");
        close(client_socket);
        return;
    }

    if (recv_all(client_socket, buffer_cifrado, file_size) != 0) {
        fprintf(stderr, "[HANDLER] Error al recibir los datos.\n");
    } else {
        printf("[HANDLER] Datos cifrados recibidos correctamente.\n");

        // 3. Guardar el archivo cifrado (Requisito del proyecto)
        FILE *cifrado_file = fopen("archivo_recibido.cif", "wb");
        if (cifrado_file) {
            fwrite(buffer_cifrado, 1, file_size, cifrado_file);
            fclose(cifrado_file);
            printf("[HANDLER] Archivo cifrado guardado en 'archivo_recibido.cif'.\n");
        }

        // 4. Pasar los datos cifrados al gestor de nodos para que los procese
        procesar_datos_distribuidos(buffer_cifrado, file_size, key);
    }

    // 5. Limpieza
    free(buffer_cifrado);
    close(client_socket);
    printf("[HANDLER] Cliente desconectado.\n");
}

int main(int argc, char *argv[]) {
    MPI_Init(&argc, &argv);
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    if (rank == 0){
        if (argc != 3) {
            fprintf(stderr, "Uso: %s <puerto> <clave>\n", argv[0]);
            exit(EXIT_FAILURE);
        }

        int port = atoi(argv[1]);
        const char *key = argv[2];
        int server_socket;
        struct sockaddr_in server_addr;

        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) die_with_error("Error al crear el socket");

        int reuse = 1;
        if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0) {
            die_with_error("setsockopt(SO_REUSEADDR) failed");
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);

        if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            die_with_error("Error en bind");
        }

        if (listen(server_socket, 5) < 0) die_with_error("Error en listen");

        printf("[SERVIDOR] Esperando conexiones en el puerto %d...\n", port);

        for (;;) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &addr_len);
            if (client_socket < 0) {
                perror("Error en accept, continuando...");
                continue;
            }

            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
            printf("\n[SERVIDOR] Petición de conexión de %s:%d. Pasando al handler...\n", client_ip, ntohs(client_addr.sin_port));

            handle_client(client_socket, key);
            
            printf("[SERVIDOR] Esperando nueva conexión...\n");
        }
    } else{
        procesar_datos_distribuidos(NULL, 0, NULL);
    }
    close(server_socket);
    MPI_Finalize();
    return 0;
}