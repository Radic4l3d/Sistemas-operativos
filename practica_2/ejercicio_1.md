# Codifica las siguientes instrucciones y realiza las preguntas.  
`#define N_PROCESOS 3
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

int I=0;
void cod_del_proceso(int id, int t){
int i;
for(i=0;i<5;i++){
printf("\nProceso %d i= %d I= %d\n",id, i, I);
I++;
}
exit(t);
}

int main(){
pid_t pid;
int p, edo;

for(p=0;p<N_PROCESOS;p++){
pid=fork();
if(pid==-1){
perror("Error en fork");
exit(-1);
}
else if(pid==0){
cod_del_proceso(getpid(), p+1);
}else{
printf("\n\t\tSoy el padre mi PID %d PPID %d pid hijo %d\n", getpid(), getppid(), pid);
}
}

for(p=0;p<N_PROCESOS;p++){

pid=wait(&edo);

printf("\nTermino el proceso %d con estado %d\n", pid, edo>>8);

}

exit(0);
}`  

1. Coloca las instrucciones necesarias para que en otra terminal muestres una captura del árbol de procesos donde se muestren los procesos creados.  
2. ¿Qué hace el programa?  
3. Agrega al código una variable local al main, has que cada proceso la modifique en el main (padre, hijos), e imprimela en pantalla.  
4. ¿Los procesos comparten esta variable y/o la variable i de la función cod_del_proceso y/o la variable global I ?  
