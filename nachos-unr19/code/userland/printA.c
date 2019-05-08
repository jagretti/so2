
#include "syscall.h"
#include<stdio.h>
#include<stdlib.h>
#include <unistd.h>

int main(void)
{
    Create("A.txt");
    OpenFileId o = Open("A.txt");
    for(;;) {
        Write("A\n",sizeof "A\n", o);
    } 
}
