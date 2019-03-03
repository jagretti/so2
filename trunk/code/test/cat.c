
#include "syscall.h"

int main(int argc, char** argv)
{
  if (argc < 1) Exit(1);
  OpenFileId output = ConsoleOutput;
  OpenFileId input =  Open(argv[0]);
  char buff[1];
  int read = Read(buff, 1, input);
  while (read == 1) {
    Write(buff, 1, output);
    read = Read(buff, 1, input);
  }
  // EOF
  buff[0] = '\0';
  Write(buff, 1, output);
  Close(output);
  Close(input);
}
