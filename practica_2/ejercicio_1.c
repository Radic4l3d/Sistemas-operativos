#define N_PROCESOS 3
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int I=0;
void cod_del_proceso(int id, int t){
  int i;
  for(i=0;i<5;i++){
    printf("Proceso hijo %d i= %d I= %d\n",id, i, I);
    I++;
  }
  exit(t);
}

int main(){
    pid_t pid;
    int p, edo;
    int variable_local = 100; // <--- Declaramos la variable local en main con valor inicial 100

    for(p=0;p<N_PROCESOS;p++){
      pid=fork();
      if(pid==-1){
        perror("Error en fork");
        exit(-1);
      }
      else if(pid==0){
        variable_local = variable_local + getpid();
        printf("----> SOY HIJO (PID %d), mi variable_local ahora es = %d\n", getpid(), variable_local);
        cod_del_proceso(getpid(), p+1);
      }else{
        variable_local = variable_local - 1;
        printf("\t\tPADRE (PID %d): mi variable_local ahora es = %d. Acabo de crear a HIJO (PID %d)\n", getpid(), variable_local, pid);
      }
    }

    for(p=0;p<N_PROCESOS;p++){
      pid=wait(&edo);
      printf("\nTermino el proceso %d con estado %d\n", pid, edo>>8);
    }

    // El padre imprime su valor FINAL
    printf("\n====> PADRE (PID %d) al FINAL: mi variable_local es = %d\n", getpid(), variable_local);

    exit(0);
}
