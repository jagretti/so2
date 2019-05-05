
#include "syscall.h"
#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>

int main(void)
{
    Create("printA.txt");
    OpenFileId o = Open("printA.txt");
    for(;;) {
        Write("A\n",sizeof "A\n", o);
    } 
}
