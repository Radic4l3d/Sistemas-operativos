Realiza un programa en c que cree dos procesos hijos, cada uno de estos procesos creados cree tres procesos hijos y a su vez cada uno de estos cree dos procesos hijos.  
Deberás sincronizar los procesos. Denominaremos generaciones de los procesos dependiendo en qué nivel del sub árbol de procesos se encuentren.  
El primer proceso se encuentra en la generación X, los hijos de este proceso en la generación Millenial, los hijos de esta generación estarán en la generación Z,  
y finalmente los hijos de esta generación estarán en la generación alfa.  
Cada proceso en este ejercicio deberá imprimir su pid, el pid de su padre y la generación en la que se encuentra.  
También, los procesos padre deberán imprimir un mensaje cuando terminen de ejecutarse cada uno de sus hijos, este mensaje deberá incluir el pid del proceso que termino.  
En otra terminal imprime el árbol de procesos del sistema y realiza una captura de pantalla donde se muestre el sub árbol de procesos creado en este ejercicio.  
En la siguiente imagen se ve como es el árbol de procesos de este ejercicio:

![Arbol](/arbol.png)
