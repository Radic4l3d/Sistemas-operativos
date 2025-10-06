/* ejercicio2.c
   Estructura: X -> 2 hijos (Millenial)
               Millenial -> 3 hijos (Z)
               Z -> 2 hijos (Alfa)
   Cada proceso imprime PID, PPID y generación.
   Cada padre imprime un mensaje cuando termina cada hijo.
   Las hojas (Alfa) hacen sleep para que puedas ver el árbol con pstree.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int children_counts[] = {2, 3, 2, 0};      // número de hijos por nivel: X, Millenial, Z, Alfa
const char *gen_names[] = {"X", "Millenial", "Z", "Alfa"};
#define SLEEP_TIME 10  // segundos que duerme la generación Alfa para permitir captura del árbol

void spawn_children(int level) {
    int num = children_counts[level];
    if (num == 0) {
        /* Hoja (Alfa): no crea más hijos; dormimos para que el árbol sea visible */
        sleep(SLEEP_TIME);
        return;
    }

    pid_t pid;
    int status;

    for (int i = 0; i < num; i++) {
        pid = fork();
        if (pid < 0) {
            perror("fork");
            exit(1);
        } else if (pid == 0) {
            /* Soy el hijo: imprimo mi info y continúo creando mi generación siguiente */
            printf("PID: %d | PPID: %d | Generación: %s\n", getpid(), getppid(), gen_names[level+1]);
            fflush(stdout);
            spawn_children(level + 1);
            /* cuando termino mis descendientes, salgo */
            exit(0);
        }
        /* el padre sigue creando más hijos en este bucle */
    }

    /* Padre del bucle: espera a cada hijo y reporta cuando termina */
    pid_t w;
    while ((w = wait(&status)) > 0) {
        printf("(Padre PID %d) Mi hijo %d terminó con estado %d\n", getpid(), w, WEXITSTATUS(status));
        fflush(stdout);
    }
}

int main(void) {
    /* Proceso raíz (generación X) */
    printf("PID: %d | PPID: %d | Generación: %s (raíz)\n", getpid(), getppid(), gen_names[0]);
    fflush(stdout);

    spawn_children(0);

    printf("Proceso raíz (PID %d) ha terminado\n", getpid());
    return 0;
}
