#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include "comms.h" // Incluimos la cabecera común

int main() {
    key_t key;
    int shmid;
    struct comms_data *shm_ptr;
    pid_t pid_b;
    int opcion;

    // 1. Generar la clave IPC (la misma que B)
    key = ftok(KEY_PATHNAME, KEY_ID);
    if (key == -1) {
        perror("[A] ftok");
        exit(1);
    }

    // 2. Obtener el ID de la memoria compartida
    shmid = shmget(key, sizeof(struct comms_data), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("[A] shmget");
        exit(1);
    }

    // 3. Mapear (attach) la memoria compartida
    shm_ptr = (struct comms_data *)shmat(shmid, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("[A] shmat");
        exit(1);
    }

    // 4. Esperar a que B escriba su PID
    printf("[A] Esperando que Proceso B se conecte y escriba su PID...\n");
    while ((pid_b = shm_ptr->pid_b) == 0) {
        sleep(1);
    }
    printf("[A] Proceso B conectado (PID: %d).\n\n", pid_b);

    // 5. Bucle del Menú
    while (1) {
        printf("--- Menú de Control (Proceso A) ---\n");
        printf("1. Pausar Proceso B\n");
        printf("2. Reanudar Proceso B\n");
        printf("3. Enviar Mensaje a Proceso B\n");
        printf("4. Terminar Proceso B y Salir\n");
        printf("Elige una opción: ");

        if (scanf("%d", &opcion) != 1) {
            // Limpiar buffer de entrada si no es un número
            while (getchar() != '\n');
            opcion = 0;
        }

        switch (opcion) {
            case 1: // Pausar (SIGSTOP)
                printf("[A] Enviando señal SIGSTOP a %d...\n", pid_b);
                if (kill(pid_b, SIGSTOP) == -1) perror("[A] kill SIGSTOP");
                break;

            case 2: // Reanudar (SIGCONT)
                printf("[A] Enviando señal SIGCONT a %d...\n", pid_b);
                if (kill(pid_b, SIGCONT) == -1) perror("[A] kill SIGCONT");
                break;

            case 3: // Mensaje (SIGUSR1)
                printf("[A] Escribe el mensaje: ");
                // Consumir el '\n' pendiente del scanf anterior
                while (getchar() != '\n'); 
                // Leer el mensaje (incluyendo espacios)
                if (fgets(shm_ptr->mensaje, 256, stdin) != NULL) {
                    // Quitar el salto de línea de fgets
                    shm_ptr->mensaje[strcspn(shm_ptr->mensaje, "\n")] = 0;
                }
                
                printf("[A] Enviando señal SIGUSR1 a %d...\n", pid_b);
                if (kill(pid_b, SIGUSR1) == -1) perror("[A] kill SIGUSR1");
                break;

            case 4: // Terminar (SIGTERM)
                printf("[A] Enviando señal SIGTERM a %d...\n", pid_b);
                if (kill(pid_b, SIGTERM) == -1) perror("[A] kill SIGTERM");

                // Esperar un poco a que B termine
                sleep(1); 
                
                // Limpiar (des-mapear y eliminar)
                printf("[A] Limpiando memoria compartida y saliendo.\n");
                if (shmdt(shm_ptr) == -1) perror("[A] shmdt");
                if (shmctl(shmid, IPC_RMID, NULL) == -1) perror("[A] shmctl IPC_RMID");
                exit(0);

            default:
                printf("[A] Opción no válida. Inténtalo de nuevo.\n");
                break;
        }
        printf("\n");
    }

    return 0;
}
