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
main(int argc, char **argv)
{
    // Valido que sean dos argumentos
    if (argc < 3) {
        Write("Argumentos invalidos\n", 22, ConsoleOutput);
        Write("USO CORRECTO: $cp archivo_origen archivo_destino\n", 52, ConsoleOutput);
        Exit(0);
    }
        
    char *src_file = argv[1];
    char *dst_file = argv[2];

    //Abro archivo de origen
    int srcFileId = Open(src_file);
    if (srcFileId < 1) {
        Write("El archivo de origen no existe\n", 34, ConsoleOutput);
        Exit(0);
    }

    // Los argumentos son dos, uso el 2do para ver si ya existe o hay que crear el archivo
    int dstFileId = Open(dst_file);
    if (dstFileId < 1) {
        Create(dst_file);
        dstFileId = Open(dst_file);
    }
    char buffer[1];
    int read = Read(buffer, 1, srcFileId);
    while (read == 1) {
        Write(buffer, 1, dstFileId);
        read = Read(buffer, 1, srcFileId);
    }
    Close(srcFileId);
    Close(dstFileId);

}
