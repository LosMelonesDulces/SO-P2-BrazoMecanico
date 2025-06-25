#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>     // Para close()
#include <arpa/inet.h>  // Para inet_pton, htons, etc.
#include <sys/socket.h> // Para socket, connect, send, recv

// --- INICIO DE LA LÓGICA DE CIFRADO (del paso anterior) ---

void xor_crypt_inplace(const char *key, unsigned char *data, size_t data_len) {
    size_t key_len = strlen(key);
    for (size_t i = 0; i < data_len; ++i) {
        data[i] = data[i] ^ key[i % key_len];
    }
}

unsigned char* cifrar_archivo_a_memoria(const char *filepath, const char *key, size_t *output_size) {
    FILE *file = fopen(filepath, "rb");
    if (!file) {
        perror("Error: No se pudo abrir el archivo de entrada");
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    rewind(file);
    if (file_size < 0) {
        perror("Error: No se pudo determinar el tamaño del archivo");
        fclose(file);
        return NULL;
    }
    unsigned char *buffer = malloc(file_size);
    if (!buffer) {
        fprintf(stderr, "Error: No se pudo alojar memoria para el archivo\n");
        fclose(file);
        return NULL;
    }
    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != (size_t)file_size) {
        fprintf(stderr, "Error: Ocurrió un error al leer el archivo\n");
        fclose(file);
        free(buffer);
        return NULL;
    }
    fclose(file);
    xor_crypt_inplace(key, buffer, bytes_read);
    *output_size = bytes_read;
    return buffer;
}
// --- FIN DE LA LÓGICA DE CIFRADO ---


int main(int argc, char const *argv[]) {
    if (argc != 5) {
        fprintf(stderr, "Uso: %s <IP servidor> <puerto> <archivo> <clave>\n", argv[0]);
        return 1;
    }

    const char *server_ip = argv[1];
    int port = atoi(argv[2]);
    const char *filepath = argv[3];
    const char *key = argv[4];
    
    // --- 1. Cifrar el archivo en memoria ---
    printf("[CLIENTE] Cifrando el archivo '%s'...\n", filepath);
    size_t encrypted_size = 0;
    unsigned char *encrypted_data = cifrar_archivo_a_memoria(filepath, key, &encrypted_size);

    if (encrypted_data == NULL) {
        fprintf(stderr, "[CLIENTE] Falló el cifrado del archivo.\n");
        return 1;
    }
    printf("[CLIENTE] Cifrado exitoso. Tamaño: %zu bytes.\n", encrypted_size);

    // --- 2. Preparar la conexión del socket ---
    int client_socket;
    struct sockaddr_in server_addr;

    // Crear el socket
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0) {
        perror("[CLIENTE] Error al crear el socket");
        free(encrypted_data);
        return 1;
    }

    // Configurar la dirección del servidor
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // Convertir la dirección IP de texto a binario
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        perror("[CLIENTE] Dirección IP inválida");
        close(client_socket);
        free(encrypted_data);
        return 1;
    }

    // --- 3. Conectar al servidor ---
    printf("[CLIENTE] Conectando a %s:%d...\n", server_ip, port);
    if (connect(client_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("[CLIENTE] Falla en la conexión");
        close(client_socket);
        free(encrypted_data);
        return 1;
    }
    printf("[CLIENTE] Conexión establecida.\n");

    // --- 4. Enviar los datos ---
    // Primero, enviar el tamaño del archivo para que el servidor sepa cuántos bytes esperar.
    // Se convierte a formato de red para evitar problemas de endianness.
    uint64_t net_size = htobe64(encrypted_size); // a 64-bit network byte order
    if (send(client_socket, &net_size, sizeof(net_size), 0) < 0) {
        perror("[CLIENTE] Error al enviar el tamaño del archivo");
    } else {
        printf("[CLIENTE] Enviando %zu bytes de datos...\n", encrypted_size);
        // Segundo, enviar los datos cifrados
        if (send(client_socket, encrypted_data, encrypted_size, 0) < 0) {
            perror("[CLIENTE] Error al enviar los datos del archivo");
        } else {
            printf("[CLIENTE] Datos enviados exitosamente.\n");
        }
    }

    // --- 5. Limpieza ---
    printf("[CLIENTE] Liberando memoria y cerrando conexión.\n");
    free(encrypted_data); // Liberar la memoria del buffer de cifrado
    close(client_socket); // Cerrar el socket

    return 0;
}