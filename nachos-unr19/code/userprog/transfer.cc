#include "transfer.hh"
#include "lib/utility.hh"
#include "threads/system.hh"


bool ReadStringFromUser(int userAddress, char *outString,
                        unsigned maxByteCount)
{
    ASSERT(userAddress != 0);
    ASSERT(outString != nullptr);
    ASSERT(maxByteCount != 0);

    unsigned count = 0;
    do {
        int temp;
        count++;
        bool read = machine->ReadMem(userAddress, 1, &temp);
        DEBUG('a', "Lectura ReadMem %d\n", read);
        if(read) {
            DEBUG('o', "ReadStringFromUser %c\n", (unsigned char) temp);
            *outString = (unsigned char) temp;
        } else {
            read = machine->ReadMem(userAddress, 1, &temp);
            DEBUG('o', "ReadStringFromUser %c\n", (unsigned char) temp);
            *outString = (unsigned char) temp;
        }
        userAddress++;
    } while (*outString++ != '\0' && count < maxByteCount);

    return *(outString - 1) == '\0';
}

void
ReadBufferFromUser(int usrAddr, char *outBuff, unsigned byteCount)
{
    int c, i = 0;
    while (byteCount > 0) {
        //ASSERT(machine->ReadMem(usrAddr+i,1,&c));
        bool read = machine->ReadMem(usrAddr+i,1,&c);
        if(read) {
            outBuff[i] = c;
        } else {
            read = machine->ReadMem(usrAddr+i,1,&c);
            outBuff[i] = c;
        }
        //outBuff[i++] = c;
        i++;
        byteCount--;
    }
}

void
WriteStringToUser(const char *str, int usrAddr)
{
    int c, i = 0;
    while ((c = str[i]) != '\0') {
        //ASSERT(machine->WriteMem(usrAddr+i,1,c));
        bool write = machine->WriteMem(usrAddr+i,1,c);
        if(!write) {
            machine->WriteMem(usrAddr+i,1,c);
        }
        i++;
    }
}

void
WriteBufferToUser(const char *str, int userAddress, unsigned int byteCount)
{
    int i = 0;
    while (byteCount > 0) {
        //ASSERT(machine -> WriteMem(usrAddr+i,1,str[i]));
        bool write = machine -> WriteMem(userAddress+i,1,str[i]);
        if(!write) {
            machine -> WriteMem(userAddress+i,1,str[i]);
        }
        byteCount--;
        i++;
    }
}
