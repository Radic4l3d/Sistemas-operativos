## Investiga cuáles son los operadores de atributos de archivos en bash y qué hacen. 
 
 
En Bash, los operadores de atributos de archivos, también conocidos como operadores de 
prueba de archivos, se utilizan dentro de expresiones condicionales (generalmente con if o 
test/[]) para verificar diversas propiedades de un archivo o directorio. 
 
### Operadores de Atributos de Archivos 
Estos operadores te permiten comprobar si un archivo existe, qué tipo de archivo es y los 
permisos que tiene. 

- *Operador* 
- *Descripción* 
- *Ejemplo de Uso*

*-e*
Existe: Devuelve verdadero si el archivo o directorio 
if [ -e archivo.txt ] 

-f 
Es un archivo regular: Devuelve verdadero si la ruta corresponde a un archivo normal. 
if [ -f documento.pdf ] 

-d 
Es un directorio: Devuelve verdadero si la ruta corresponde a un directorio. 
if [ d ./mi_carpeta ] 

-s 
No está vacío: 
Devuelve verdadero si el archivo existe y tiene un tamaño mayor que cero. 
if [ -s datos.csv ] 

-r 
Tiene permiso de lectura: Devuelve verdadero si el archivo tiene permiso de lectura para el usuario actual. 
if [ -r config.conf ] 

-w 
Tiene permiso de escritura: Devuelve verdadero si el archivo tiene permiso de escritura para el usuario actual. 
if [ -w log.txt ] 

-x 
Tiene permiso de ejecución: Devuelve verdadero si el archivo tiene permiso de ejecución para el usuario actual. 
if [ -x mi_script.sh ]

-L o -h 
Es un enlace simbólico: Devuelve verdadero si la ruta es un enlace simbólico. 
if [ -L enlace ] 
 
 
 
 
 
 
 
## Operadores de Comparación de Archivos
Estos operadores se utilizan para comparar dos archivos entre sí. 
- *Operador* 
- *Descripción* 
- *Ejemplo de Uso*

f1 -nt f2 
Más nuevo que (newer than): Devuelve verdadero si el archivo f1 fue modificado más recientemente que el archivo f2. 
if [ archivo1.txt -nt archivo2.txt ] 

f1 -ot f2 
Más antiguo que (older than): Devuelve verdadero si el archivo f1 es más antiguo que el archivo f2. 
if [ registro.log -ot backup.log ] 

f1 -ef f2 
Mismo archivo (same file): Devuelve verdadero si f1 y f2 son enlaces duros al mismo archivo (tienen el mismo número de inodo). 
if [ archivo -ef enlace_duro ] 
