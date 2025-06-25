#!/bin/bash
# Este script compila y luego ejecuta el programa cliente.

# --- Validación de argumentos ---
if [ "$#" -ne 4 ]; then
    echo "Error: Debes proporcionar la IP del servidor, el puerto, el archivo y la clave."
    echo "Uso: ./cliente.sh <IP_servidor> <puerto> <archivo> <clave_secreta>"
    exit 1
fi

# Asignar los argumentos del script a variables para mayor claridad
IP_SERVIDOR=$1
PUERTO=$2
ARCHIVO=$3
CLAVE=$4

echo "Paso 1: Compilando cliente.c..."

# --- Compilación ---
# Compila el código fuente del cliente y crea un ejecutable llamado 'cliente'
gcc -Wall -g -o cliente cliente.c

# --- Ejecución ---
# Verifica si la compilación fue exitosa (código de salida 0)
if [ $? -eq 0 ]; then
    echo "¡Compilación exitosa!"
    echo "-------------------------------------"
    echo "Paso 2: Ejecutando cliente..."

    # Ejecuta el programa cliente con los argumentos pasados al script
    ./cliente "$IP_SERVIDOR" "$PUERTO" "$ARCHIVO" "$CLAVE"
else
    # Mensaje en caso de que la compilación falle
    echo "¡Error de compilación! Revisa el código."
fi