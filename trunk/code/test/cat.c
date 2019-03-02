#include "syscall.h"

int main(int argc, char** argv)
{
  if (argc < 1) Exit(1); // without arg nothing to do.
  OpenFileId output = ConsoleOutput;
  OpenFileId inputFile = Open(argv[0]);
  char buff[1];
  Read(buff, 1, inputFile);
  while (buff[0] != "\0") {
    Write(buff, 1, output);
    Read(buff, 1, inputFile);
  }
  Exit(0);
}
