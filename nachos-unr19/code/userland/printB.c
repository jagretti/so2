
#include "syscall.h"
#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>

int main(void)
{
    Create("B.txt");
    OpenFileId o = Open("B.txt");
    for(;;) {
        Write("B\n",sizeof "B\n", o);
    } 
}
