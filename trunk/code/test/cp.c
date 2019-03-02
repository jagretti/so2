/* Programa de usuario cp
*
* Copia el archivo de origen, poniendole el nombre del archivo destino
*
* USO: cp archivo_origen archivo_destino
*
* Si el destino no existe, lo crea
*
*/

#include "syscall.h"

#define SIZE 1024

int 
main(int argc, char *argv)
{
    // Valido que sean dos argumentos
    if (argc < 2) {
        Write("Argumentos invalidos\n", 22, ConsoleOutput);
        Write("USO CORRECTO: $cp archivo_origen archivo_destino\n", 52, ConsoleOutput);
        Exit(0);
    }
        
    char *src_file = argv[0];
    char *dst_file = argv[1];

    //Abro archivo de origen
    int srcFileId = Open(src_file);
    if (srcFileId < 2) {
        Write("El archivo de origen no existe\n", 34, ConsoleOutput);
        Exit(0);
    }
    char *buffer[SIZE];
    Read(buffer, SIZE, srcFileId);

    // Los argumentos son dos, uso el 2do para ver si ya existe o hay que crear el archivo
    int dstFileId = Open(dst_file);
    if (dstFileId > 1) {
        Write(buffer, SIZE, dstFileId);
    } else {
        Create(dst_file);
        dstFileId = Open(dst_file);
        Write(buffer, SIZE, dstFileId);
    }
    Exit(1);

}

