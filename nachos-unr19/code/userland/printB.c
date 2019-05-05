
#include "syscall.h"
#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>

int main(void)
{
    Create("printB.txt");
    OpenFileId o = Open("printB.txt");
    for(;;) {
        Write("B\n",sizeof "B\n", o);
    } 
}
