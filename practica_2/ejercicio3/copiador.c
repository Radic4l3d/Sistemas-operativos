#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#define TAMANIO_BUFFER 4096 // Tamaño del buffer para leer/escribir (4KB)

int main(int argc, char *argv[]) {
    // Validar el número de parámetros
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <archivo_origen> <archivo_destino>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *archivo_origen = argv[1];
    const char *archivo_destino = argv[2];
    int fd_origen, fd_destino;
    ssize_t bytes_leidos;
    char buffer[TAMANIO_BUFFER];

    // Abrir el archivo de origen para lectura
    fd_origen = open(archivo_origen, O_RDONLY);
    if (fd_origen == -1) {
        perror("Error al abrir el archivo de origen");
        exit(EXIT_FAILURE);
    }

    fd_destino = open(archivo_destino, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (fd_destino == -1) {
        perror("Error al crear/abrir el archivo de destino");
        close(fd_origen); // Cerrar el descriptor ya abierto
        exit(EXIT_FAILURE);
    }

    while ((bytes_leidos = read(fd_origen, buffer, TAMANIO_BUFFER)) > 0) {
        if (write(fd_destino, buffer, bytes_leidos) != bytes_leidos) {
            close(fd_origen);
            close(fd_destino);
            exit(EXIT_FAILURE);
        }  perror("Error al escribir en el archivo de destino");
          
    }
    
    if (bytes_leidos == -1) {
        perror("Error al leer el archivo de origen");
    }

    close(fd_origen);
    close(fd_destino);

    printf("Copia completada: '%s' -> '%s'\n", archivo_origen, archivo_destino);
    return 0;
}
