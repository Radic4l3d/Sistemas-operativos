// Archivo: procesoA.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#define FIFO_PATH "/tmp/pid_fifo"

void mostrar_menu() {
    printf("\n--- MENU DE CONTROL ---\n");
    printf("1. Pausar proceso B (SIGSTOP)\n");
    printf("2. Reanudar proceso B (SIGCONT)\n");
    printf("3. Enviar mensaje a proceso B (SIGUSR1)\n");
    printf("4. Terminar proceso B (SIGTERM)\n");
    printf("5. Salir (y dejar B corriendo)\n");
    printf("Elige una opción: ");
}

int main() {
    // --- Creación y Configuración del FIFO ---
    // Si el FIFO ya existe, lo ignoramos.
    if (mkfifo(FIFO_PATH, 0666) == -1 && errno != EEXIST) {
        perror("[Proceso A] Error al crear el FIFO");
        exit(EXIT_FAILURE);
    }

    printf("[Proceso A] Esperando a que Proceso B envíe su PID...\n");
    printf("[Proceso A] Por favor, inicia el Proceso B en otra terminal.\n");

    // Abrimos el FIFO en modo lectura. Esta llamada se bloqueará
    // hasta que el Proceso B lo abra para escribir.
    int fd = open(FIFO_PATH, O_RDONLY);
    if (fd == -1) {
        perror("[Proceso A] Error al abrir el FIFO");
        exit(EXIT_FAILURE);
    }

    // Leemos el PID que nos envía el Proceso B
    pid_t pid_b;
    if (read(fd, &pid_b, sizeof(pid_b)) == -1) {
        perror("[Proceso A] Error al leer del FIFO");
        close(fd);
        exit(EXIT_FAILURE);
    }

    // Una vez leído el PID, ya no necesitamos la tubería.
    close(fd);
    unlink(FIFO_PATH); // Eliminamos el archivo FIFO del sistema

    printf("\n[Proceso A] PID del Proceso B recibido: %d. Listo para controlar.\n", pid_b);

    // --- Bucle del Menú ---
    int opcion;
    do {
        mostrar_menu();
        // Leemos la entrada del usuario
        if (scanf("%d", &opcion) != 1) {
            // Limpiamos el buffer de entrada en caso de error (e.g., si se introduce texto)
            while(getchar() != '\n');
            opcion = 0; // Forzamos una opción inválida
        }

        switch (opcion) {
            case 1:
                printf("Enviando señal SIGSTOP para pausar a B (%d)...\n", pid_b);
                if (kill(pid_b, SIGSTOP) == -1) perror("Error al enviar SIGSTOP");
                break;
            case 2:
                printf("Enviando señal SIGCONT para reanudar a B (%d)...\n", pid_b);
                if (kill(pid_b, SIGCONT) == -1) perror("Error al enviar SIGCONT");
                break;
            case 3:
                printf("Enviando señal SIGUSR1 para mensaje en B (%d)...\n", pid_b);
                if (kill(pid_b, SIGUSR1) == -1) perror("Error al enviar SIGUSR1");
                break;
            case 4:
                printf("Enviando señal SIGTERM para terminar a B (%d)...\n", pid_b);
                if (kill(pid_b, SIGTERM) == -1) {
                    perror("Error al enviar SIGTERM");
                } else {
                    printf("Ambos procesos finalizarán.\n");
                    sleep(1); // Damos tiempo a B para que imprima su mensaje de despedida
                    exit(EXIT_SUCCESS);
                }
                break;
            case 5:
                printf("Saliendo. El Proceso B continuará su ejecución.\n");
                break;
            default:
                printf("Opción no válida. Inténtalo de nuevo.\n");
                break;
        }

    } while (opcion != 4 && opcion != 5);

    return 0;
}
