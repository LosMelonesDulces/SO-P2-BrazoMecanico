#ifndef NODE_MANAGER_H
#define NODE_MANAGER_H

#include <stddef.h> // Para size_t

/**
 * @brief Simula la distribución de datos cifrados a los nodos de procesamiento.
 * * Esta función toma los datos cifrados, simula su división y envío a los nodos,
 * simula la recepción de resultados y la obtención de la palabra final.
 * * @param datos_cifrados Puntero al buffer con los datos cifrados.
 * @param tamano_datos El tamaño del buffer de datos.
 * @param clave La clave necesaria para que los nodos puedan descifrar.
 */
void procesar_datos_distribuidos(const unsigned char *datos_cifrados, size_t tamano_datos, const char *clave);

#endif // NODE_MANAGER_H