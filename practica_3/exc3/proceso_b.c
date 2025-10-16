// Archivo: procesoB.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

// La tubería que usaremos para comunicar el PID
#define FIFO_PATH "/tmp/pid_fifo"

// Variable global para controlar el bucle principal.
// 'volatile sig_atomic_t' es el tipo correcto para variables
// modificadas por un manejador de señales.
volatile sig_atomic_t terminar = 0;

// Manejador para la señal SIGUSR1 (opción 3 del menú)
void manejador_mensaje(int signum) {
    // NOTA: printf no es 100% seguro en manejadores de señales,
    // pero para este ejemplo es aceptable. La forma más segura
    // sería usar la llamada al sistema write().
    char msg[] = "\n[Proceso B] Mensaje recibido del Proceso A.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

// Manejador para la señal SIGTERM (opción 4 del menú)
void manejador_terminar(int signum) {
    char msg[] = "\n[Proceso B] Señal de terminación recibida. Adiós.\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    terminar = 1; // Cambia la bandera para salir del bucle
}

int main() {
    pid_t mi_pid = getpid();
    printf("[Proceso B] Iniciado con PID: %d\n", mi_pid);

    // --- Comunicación del PID via FIFO ---
    // Esperamos a que el proceso A cree la tubería
    int fd;
    printf("[Proceso B] Abriendo FIFO para enviar mi PID...\n");
    // Abrimos en modo escritura. Se bloqueará si A no ha abierto para leer.
    fd = open(FIFO_PATH, O_WRONLY);
    if (fd == -1) {
        perror("[Proceso B] Error al abrir el FIFO");
        exit(EXIT_FAILURE);
    }

    // Escribimos nuestro PID en el FIFO
    if (write(fd, &mi_pid, sizeof(mi_pid)) == -1) {
        perror("[Proceso B] Error al escribir en el FIFO");
        close(fd);
        exit(EXIT_FAILURE);
    }
    close(fd);
    printf("[Proceso B] PID enviado a Proceso A. Iniciando contador.\n");

    // --- Configuración de Señales ---
    signal(SIGUSR1, manejador_mensaje);
    signal(SIGTERM, manejador_terminar);

    // --- Bucle Principal del Contador ---
    long long contador = 0;
    while (!terminar) {
        printf("Contador: %lld\n", contador);
        contador++;
        sleep(1);
    }

    return 0;
}
