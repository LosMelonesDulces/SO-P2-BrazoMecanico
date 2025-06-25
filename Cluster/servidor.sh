#!/bin/bash
# Este script compila todos los archivos .c del servidor y lo ejecuta.

if [ "$#" -ne 2 ]; then
    echo "Uso: ./compilar_servidor.sh <puerto> <clave>"
    exit 1
fi

PUERTO=$1
CLAVE=$2

echo "Compilando servidor_final.c y node_manager.c..."

# Compilar todos los archivos .c juntos para crear un único ejecutable
gcc -Wall -g -o servidor servidor.c node_manager.c

if [ $? -eq 0 ]; then
    echo "¡Compilación exitosa!"
    echo "-------------------------------------"
    echo "Iniciando servidor en el puerto $PUERTO..."
    
    ./servidor "$PUERTO" "$CLAVE"
else
    echo "¡Error de compilación!"
fi