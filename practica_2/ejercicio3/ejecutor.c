#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Uso: %s <archivo_origen> <archivo_destino>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    printf("PADRE (PID: %d): Voy a crear un hijo para que copie los archivos.\n", getpid());
    
    pid_t pid = fork();

    if (pid == -1) {
        // Error en fork
        perror("Error en fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        // PROCESO HIJO
        printf("HIJO (PID: %d): Me han creado. Voy a ejecutar el programa 'copiador'.\n", getpid());
        // Reemplaza el proceso hijo con el programa "copiador"
        char *args[] = {"./copiador", argv[1], argv[2], NULL};

        execvp(args[0], args);
        // Si execvp retorna, significa que hubo un error
        perror("Error en execvp");
        exit(127);
    } else {
        // PROCESO PADRE 
        int status;

        // El padre espera a que el proceso hijo termine
        wait(&status);
        printf("PADRE (PID: %d): Mi hijo ha terminado su tarea.\n", getpid());

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("PADRE: La copia se realizó con éxito.\n");
        } else {
            fprintf(stderr, "PADRE: El proceso de copia falló.\n");
        }
    }

    return 0;
}
