#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h> // Cabecera principal de OpenMPI
#include "node_manager.h"

// --- Función de ayuda para descifrar (usada por los workers) ---
void xor_decrypt_inplace(const char *key, unsigned char *data, size_t data_len) {
    size_t key_len = strlen(key);
    for (size_t i = 0; i < data_len; ++i) {
        data[i] = data[i] ^ key[i % key_len];
    }
}

// --- Estructura para contar palabras ---
typedef struct WordCount {
    char *word;
    int count;
    struct WordCount *next;
} WordCount;

// --- Función de ayuda para encontrar la palabra más frecuente (usada por los workers) ---
// NOTA: Esta es una implementación simple con listas enlazadas.
void find_most_frequent_word(const unsigned char* text, size_t len, char* most_frequent, int* max_count) {
    WordCount *head = NULL;
    char *text_copy = malloc(len + 1);
    if (!text_copy) return;
    memcpy(text_copy, text, len);
    text_copy[len] = '\0';

    const char *delimiters = " \t\n\r,.;:!?\"()[]{}";
    char *token = strtok(text_copy, delimiters);

    while (token != NULL) {
        WordCount *current = head;
        WordCount *found = NULL;
        while (current != NULL) {
            if (strcmp(current->word, token) == 0) {
                found = current;
                break;
            }
            current = current->next;
        }

        if (found) {
            found->count++;
        } else {
            WordCount *newNode = malloc(sizeof(WordCount));
            newNode->word = strdup(token);
            newNode->count = 1;
            newNode->next = head;
            head = newNode;
        }
        token = strtok(NULL, delimiters);
    }

    *max_count = 0;
    strcpy(most_frequent, "");
    WordCount *current = head;
    while (current != NULL) {
        if (current->count > *max_count) {
            *max_count = current->count;
            strcpy(most_frequent, current->word);
        }
        WordCount *temp = current;
        current = current->next;
        free(temp->word);
        free(temp);
    }
    free(text_copy);
}


// --- Función principal que implementa la lógica distribuida ---
void procesar_datos_distribuidos(const unsigned char *datos_cifrados, size_t tamano_datos, const char *clave) {
    
    int rank, num_procs;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &num_procs);

    // ================================================================
    // ===== LÓGICA DEL MANAGER (SERVIDOR, RANK 0) ====================
    // ================================================================
    if (rank == 0) {
        printf("[MANAGER] Distribuyendo trabajo a %d workers...\n", num_procs - 1);
        
        size_t tamano_base = tamano_datos / (num_procs - 1);
        size_t tamano_extra = tamano_datos % (num_procs - 1);
        size_t offset_actual = 0;
        size_t key_len = strlen(clave) + 1;

        for (int i = 1; i < num_procs; ++i) {
            size_t tamano_chunk = tamano_base + (i - 1 < tamano_extra ? 1 : 0);
            
            printf("    -> Enviando %zu bytes al Worker %d\n", tamano_chunk, i);
            MPI_Send(&tamano_chunk, 1, MPI_UNSIGNED_LONG, i, 0, MPI_COMM_WORLD);
            MPI_Send(datos_cifrados + offset_actual, tamano_chunk, MPI_UNSIGNED_CHAR, i, 1, MPI_COMM_WORLD);
            MPI_Send(clave, key_len, MPI_CHAR, i, 2, MPI_COMM_WORLD);
            
            offset_actual += tamano_chunk;
        }

        char global_best_word[100] = "";
        int global_max_count = 0;

        printf("[MANAGER] Esperando resultados de los workers...\n");
        for (int i = 1; i < num_procs; ++i) {
            char local_best_word[100];
            int local_max_count;

            MPI_Recv(&local_max_count, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(local_best_word, 100, MPI_CHAR, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
            printf("    <- Resultado del Worker %d: palabra='%s', frecuencia=%d\n", i, local_best_word, local_max_count);

            if (local_max_count > global_max_count) {
                global_max_count = local_max_count;
                strcpy(global_best_word, local_best_word);
            }
        }

        printf("[MANAGER] Proceso finalizado. Palabra más repetida: '%s' (%d veces).\n", global_best_word, global_max_count);
        printf("[MANAGER] (Placeholder) Comandando al hardware para escribir '%s'...\n", global_best_word);
    }
    // ================================================================
    // ===== LÓGICA DE LOS WORKERS (NODOS, RANK > 0) ==================
    // ================================================================
    else {
        size_t mi_tamano_chunk;
        char mi_clave[100];
        
        MPI_Recv(&mi_tamano_chunk, 1, MPI_UNSIGNED_LONG, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        unsigned char* mi_chunk_cifrado = malloc(mi_tamano_chunk);
        MPI_Recv(mi_chunk_cifrado, mi_tamano_chunk, MPI_UNSIGNED_CHAR, 0, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        MPI_Status key_status;
        MPI_Recv(mi_clave, 100, MPI_CHAR, 0, 2, MPI_COMM_WORLD, &key_status);

        xor_decrypt_inplace(mi_clave, mi_chunk_cifrado, mi_tamano_chunk);
        
        char mi_palabra_frecuente[100];
        int mi_frecuencia_max;
        find_most_frequent_word(mi_chunk_cifrado, mi_tamano_chunk, mi_palabra_frecuente, &mi_frecuencia_max);
        
        MPI_Send(&mi_frecuencia_max, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(mi_palabra_frecuente, strlen(mi_palabra_frecuente) + 1, MPI_CHAR, 0, 1, MPI_COMM_WORLD);

        free(mi_chunk_cifrado);
    }
}

// La inicialización y finalización de MPI se hace en el servidor principal
// para envolver toda la ejecución de la aplicación.