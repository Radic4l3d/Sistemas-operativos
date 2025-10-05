#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

void imprimir_uso(const char *nombre_programa);

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Error: Número insuficiente de argumentos.\n");
        imprimir_uso(argv[0]);
        exit(EXIT_FAILURE);
    }

    const char *opcexec = argv[1];
    const char *comando = argv[2];

    // Validar que el comando sea "ls"
    if (strcmp(comando, "ls") != 0) {
        fprintf(stderr, "Error: Este programa solo puede ejecutar el comando 'ls'. Se recibió '%s'.\n", comando);
        imprimir_uso(argv[0]);
        exit(EXIT_FAILURE);
    }

    //CREACIÓN DEL PROCESO HIJO
    pid_t pid = fork();

    if (pid < 0) {
        // Error al crear el proceso hijo
        perror("Error en fork");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) {
        //PROCESO HIJO
        // El hijo va a llamar a exec
        printf("--- (Hijo PID: %d) Ejecutando 'ls' con %s ---\n", getpid(), opcexec);

        if (strcmp(opcexec, "-l") == 0) {
            int num_opciones_ls = argc - 3;
            switch (num_opciones_ls) {
                case 0: execl("/bin/ls", "ls", NULL); break;
                case 1: execl("/bin/ls", "ls", argv[3], NULL); break;
                case 2: execl("/bin/ls", "ls", argv[3], argv[4], NULL); break;
                case 3: execl("/bin/ls", "ls", argv[3], argv[4], argv[5], NULL); break;
                default:
                    fprintf(stderr, "La implementación de -l soporta hasta 3 opciones para ls.\n");
                    exit(127);
            }
        } else if (strcmp(opcexec, "-lp") == 0) {
            // execlp busca el comando en el PATH
            int num_opciones_ls = argc - 3;
            switch (num_opciones_ls) {
                case 0: execlp("ls", "ls", NULL); break;
                case 1: execlp("ls", "ls", argv[3], NULL); break;
                case 2: execlp("ls", "ls", argv[3], argv[4], NULL); break;
                case 3: execlp("ls", "ls", argv[3], argv[4], argv[5], NULL); break;
                default:
                    fprintf(stderr, "La implementación de -lp soporta hasta 3 opciones para ls.\n");
                    exit(127);
            }
        } else if (strcmp(opcexec, "-v") == 0) {
            execv("/bin/ls", &argv[2]);
        } else if (strcmp(opcexec, "-vp") == 0) {
            execvp("ls", &argv[2]);
        } else {
            fprintf(stderr, "Error: Opción '%s' no reconocida.\n", opcexec);
            exit(127); // Salir del hijo con un código de error
        }

        // Si exec falla, se llega aquí
        perror("Error en exec");
        exit(EXIT_FAILURE);
    } else {
        //CÓDIGO DEL PROCESO PADRE ---
        int status;
        // El padre espera a que el hijo termine
        wait(&status);
        printf("--- (Padre PID: %d) El proceso hijo ha terminado ---\n", getpid());
    }

    return 0;
}

void imprimir_uso(const char *nombre_programa) {
    fprintf(stderr, "Uso: %s <opcexec> ls [opc1] [opc2] ...\n", nombre_programa);
    fprintf(stderr, "  opcexec:\n");
    fprintf(stderr, "    -l  : Usa execl()\n");
    fprintf(stderr, "    -lp : Usa execlp()\n");
    fprintf(stderr, "    -v  : Usa execv()\n");
    fprintf(stderr, "    -vp : Usa execvp()\n");
}
