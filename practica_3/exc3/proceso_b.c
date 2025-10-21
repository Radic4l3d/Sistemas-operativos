#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include "comms.h" // Incluimos la cabecera común

// Puntero global a la memoria compartida
struct comms_data *shm_ptr;

// ---- Handlers de Señales ----

// Handler para SIGUSR1 (Imprimir Mensaje)
void handler_mensaje(int sig) {
    if (shm_ptr) {
        printf("\n[B] Mensaje de A: %s\n", shm_ptr->mensaje);
    }
    // (En algunos sistemas antiguos, necesitarías re-registrar el handler aquí)
}

// Handler para SIGTERM (Terminación Limpia)
void handler_terminar(int sig) {
    printf("\n[B] Recibida señal de terminación. Adiós.\n");
    // Des-mapear la memoria compartida
    if (shmdt(shm_ptr) == -1) {
        perror("[B] shmdt");
    }
    exit(0);
}

int main() {
    key_t key;
    int shmid;

    // 1. Configurar Handlers
    signal(SIGUSR1, handler_mensaje);
    signal(SIGTERM, handler_terminar);
    
    // (No podemos manejar SIGSTOP o SIGCONT, el kernel lo hace por nosotros)

    // 2. Generar la clave IPC
    key = ftok(KEY_PATHNAME, KEY_ID);
    if (key == -1) {
        perror("[B] ftok");
        exit(1);
    }

    // 3. Obtener el ID de la memoria compartida
    // (IPC_CREAT asegura que la crea si no existe)
    shmid = shmget(key, sizeof(struct comms_data), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("[B] shmget");
        exit(1);
    }

    // 4. Mapear (attach) la memoria compartida
    shm_ptr = (struct comms_data *)shmat(shmid, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("[B] shmat");
        exit(1);
    }

    // 5. Escribir su PID en la memoria compartida
    shm_ptr->pid_b = getpid();
    printf("[B] Proceso B iniciado (PID: %d). Esperando a A...\n", getpid());

    // 6. Iniciar el bucle del contador
    int contador = 0;
    while (1) {
        printf("[B] Contador: %d\n", contador++);
        fflush(stdout); // Asegurarse de que se imprima inmediatamente
        sleep(1);
    }

    return 0;
}
